/*
 * =============================================================================
 * HYPERCUBE SLICING LOCAL SEARCH
 * =============================================================================
 *
 * PROBLEM:
 *   Given an n-dimensional hypercube with 2^n vertices and n*2^(n-1) edges,
 *   find p hyperplanes that collectively slice all edges. An edge is "sliced"
 *   if at least one hyperplane separates its two endpoints.
 *
 * HYPERPLANE REPRESENTATION:
 *   Each hyperplane is defined by coefficients a_0, a_1, ..., a_{n-1} and bias b.
 *   A vertex x ∈ {-1,+1}^n is on the positive side if:
 *       a_0*x_0 + a_1*x_1 + ... + a_{n-1}*x_{n-1} + b > 0
 *
 *   IMPLICIT +0.5 THRESHOLD:
 *   Since coefficients are integers and coordinates are ±1, the dot product
 *   a·x + b is always an integer. Using threshold 0 would make boundary cases
 *   ambiguous. Instead, we use threshold +0.5 (equivalently: side = (val >= 1)).
 *   This ensures every vertex is strictly on one side or the other.
 *
 * SYMMETRY REDUCTION:
 *   The hypercube has symmetries we can exploit. We partition coordinates into:
 *     - "Fixed" coordinates (first nfixed): all hyperplanes use the same coefficient
 *     - "Free" coordinates (last nfree): each hyperplane has independent coefficients
 *
 *   This reduces the search space, and has other nice properties.
 *   Vertices can be grouped by how many fixed coordinates are +1 (call this pc,
 *   for "positive count", ranging 0 to nfixed).
 *
 *   REDUCED VERTEX REPRESENTATION:
 *   A reduced vertex (rv) is indexed by:
 *     rv = pc * 2^nfree + fb
 *   where pc in [0, nfixed] and fb in [0, 2^nfree) is the free-coordinate bit pattern.
 *
 *   The number of original vertices represented by rv is C(nfixed, pc) — the number
 *   of ways to choose which pc of the nfixed coordinates are +1.
 *
 *   REDUCED EDGES:
 *   Edges in the reduced graph come in two types:
 *   1. "Fixed" edges: connect (pc, fb) to (pc+1, fb), flipping a fixed coordinate
 *      Multiplicity: C(nfixed, pc) * (nfixed - pc)
 *   2. "Free" edges: connect (pc, fb) to (pc, fb') where fb' differs in one bit
 *      Multiplicity: C(nfixed, pc)
 *
 *   The sum of all multiplicities equals the original edge count n*2^(n-1).
 *
 * ALGORITHM:
 *   Local search with dynamic weight boosting:
 *   1. Start with random hyperplane coefficients
 *   2. Each iteration: pick a random hyperplane and two free coordinates,
 *      propose small random changes to both
 *   3. Accept if weighted sliced count improves (or ties with S improvement)
 *   4. Periodically boost weights of unsliced edges to escape local optima
 *   5. Restart from random if stuck for too long
 *
 *   EVENNESS HEURISTIC:
 *   Good solutions tend to have each hyperplane slicing roughly the same number
 *   of edges. We add a variance penalty to the acceptance criterion to encourage
 *   this balance.
 *
 * =============================================================================
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Configuration constants */
#define NUMFREE 4            /* Number of free (independent) coordinates per hyperplane */
#define LOCAL_RANGE 10       /* Max change per coefficient per iteration */
#define SEARCH_RANGE 40      /* Absolute bound on coefficient values */
#define EVENSLICE_WEIGHT 1.0 /* Multiplier for variance penalty in acceptance */
#define UNWEIGHTED 1         /* 1: all [reduced] edges start with weight 1; 0: weight = multiplicity */
/* UNWEIGHTED controls whether edge weights are uniform or proportional to multiplicity,
 *  corresponding to operating on the reduced or unreduced hypercube. */

/* Precomputed binomial coefficients: binom[k] = C(nfixed, k) */
static long long* binom = NULL;

/* ============================================================================= */

/* Print solution matrix: p rows, each with n coefficients + bias */
static void print_solution(int n, int p, const int* M) {
    for (int i = 0; i < p; i++) {
        for (int j = 0; j <= n; j++) {
            printf("%d%c", M[i * (n + 1) + j], j == n ? '\n' : ' ');
        }
    }
}

/* Compute which side of hyperplane a reduced vertex is on.
 * Returns 1 if dot_product + bias >= 1 (i.e., > 0.5), else 0. */
static inline int compute_side(int val) {
    return (val >= 1) ? 1 : 0;
}

/* ============================================================================= */

/*
 * Initialize or reset the search state with random coefficients.
 *
 * M[i*(n+1) + j] = coefficient j of hyperplane i (j=n is the bias term)
 * val[i*num_rv + rv] = dot product + bias for hyperplane i at reduced vertex rv
 * side[i*num_rv + rv] = which side (0 or 1) of hyperplane i vertex rv is on
 * splits_count[e] = how many hyperplanes slice reduced edge e
 * plane_slice_count[i] = how many reduced edges hyperplane i slices
 */
static void init_random_state(
    int n, int p, int nfixed, int nfree, int num_rv, int num_re,
    const int* re_from, const int* re_to, const long long* re_mult,
    int* M, long long* weights, const char* free_xsign,
    unsigned char* splits_count, int* val, unsigned char* side,
    int* plane_slice_count,
    long long* W_out, long long* S_out, int* R_out) {
    int nfree_verts = 1 << nfree;

    /* All hyperplanes share the same coefficient for fixed coordinates */
    /* This can be set to a constant instead (e.g -9) if desired*/
    int fixed_coef = (rand() % (SEARCH_RANGE * 2 + 1)) - SEARCH_RANGE;

    /* Initialize coefficient matrix */
    for (int i = 0; i < p; i++) {
        /* Fixed coordinates: shared coefficient */
        for (int j = 0; j < nfixed; j++) {
            M[i * (n + 1) + j] = fixed_coef;
        }
        /* Free coordinates: random independent coefficients */
        for (int j = nfixed; j < n; j++) {
            M[i * (n + 1) + j] = (rand() % (SEARCH_RANGE * 2 + 1)) - SEARCH_RANGE;
        }
        /* Bias term: leave at 0 */
        M[i * (n + 1) + n] = 0;
    }

    /* Reset weights (unweighted mode: 1; weighted mode: multiplicity) */
    for (int e = 0; e < num_re; e++) {
        weights[e] = UNWEIGHTED ? 1 : re_mult[e];
    }

    /* Compute val and side for each (hyperplane, reduced vertex) pair.
     *
     * For reduced vertex rv = pc * nfree_verts + fb:
     *   - pc fixed coordinates are +1, (nfixed - pc) are -1
     *   - Contribution from fixed coords: fixed_coef * (pc - (nfixed - pc))
     *                                    = fixed_coef * (2*pc - nfixed)
     *   - Free coordinates determined by bit pattern fb
     */
    for (int i = 0; i < p; i++) {
        int* Mi = M + i * (n + 1);
        int* val_i = val + i * num_rv;
        unsigned char* side_i = side + i * num_rv;

        for (int pc = 0; pc <= nfixed; pc++) {
            int fixed_contrib = fixed_coef * (2 * pc - nfixed);

            for (int fb = 0; fb < nfree_verts; fb++) {
                int rv = pc * nfree_verts + fb;

                /* Start with bias and fixed contribution */
                int tot = Mi[n] + fixed_contrib;

                /* Add free coordinate contributions */
                for (int j = 0; j < nfree; j++) {
                    tot += Mi[nfixed + j] * free_xsign[fb * nfree + j];
                }

                val_i[rv] = tot;
                side_i[rv] = compute_side(tot);
            }
        }
    }

    /* Initialize plane slice counts */
    for (int i = 0; i < p; i++) {
        plane_slice_count[i] = 0;
    }

    /* Count splits for each reduced edge and accumulate statistics
     * W indicates weighted slicing count, S slicing count, R reduced slicing count */
    long long W = 0, S = 0;
    int R = 0;

    for (int e = 0; e < num_re; e++) {
        int rv1 = re_from[e], rv2 = re_to[e];
        int cnt = 0;

        for (int i = 0; i < p; i++) {
            int slices = (side[i * num_rv + rv1] != side[i * num_rv + rv2]);
            cnt += slices;
            if (slices) {
                plane_slice_count[i]++;
            }
        }

        splits_count[e] = cnt;
        if (cnt > 0) {
            S += re_mult[e];
            W += weights[e];
            R++;
        }
    }

    *W_out = W;
    *S_out = S;
    *R_out = R;
}

/* ============================================================================= */

int main(int argc, char** argv) {
    /* Parse command line arguments */
    if (argc < 5) {
        fprintf(stderr,
                "Usage: %s n p s seed [weight_period] [weight_increment] [restart_period] [weight_cap]\n\n"
                "  n: hypercube dimension\n"
                "  p: number of hyperplanes\n"
                "  s: target number of sliced edges\n"
                "  seed: random seed\n"
                "  weight_period: iterations before boosting unsliced edge weights (default: 10*num_re)\n"
                "  weight_increment: amount to increase weights (default: 1)\n"
                "  restart_period: iterations before random restart (default: num_re^2, 0=disabled)\n"
                "  weight_cap: maximum weight value (default: 1000)\n",
                argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int p = atoi(argv[2]);
    long long s_target = atoll(argv[3]);
    unsigned int seed = (unsigned int)atoll(argv[4]);
    srand(seed);

    int nfixed = n - NUMFREE;
    int nfree = NUMFREE;

    if (nfixed < 0) {
        fprintf(stderr, "Error: n=%d must be >= NUMFREE=%d\n", n, NUMFREE);
        return 1;
    }
    if (nfree < 2) {
        fprintf(stderr, "Error: NUMFREE=%d must be >= 2 for two-coordinate moves\n", nfree);
        return 1;
    }

    /* Derived constants */
    int nfree_verts = 1 << nfree;            /* 2^nfree */
    int num_rv = (nfixed + 1) * nfree_verts; /* Number of reduced vertices */
    int E_orig = n * (1 << (n - 1));         /* Original edge count: n * 2^(n-1) */

    /* Precompute binomial coefficients C(nfixed, k) for k = 0..nfixed */
    binom = malloc(sizeof(long long) * (nfixed + 1));
    binom[0] = 1;
    for (int k = 1; k <= nfixed; k++) {
        binom[k] = binom[k - 1] * (nfixed - k + 1) / k;
    }

    /* Count reduced edges:
     * - Fixed edges: nfixed * nfree_verts (one set per pc value 0..nfixed-1)
     * - Free edges: (nfixed+1) * (nfree_verts/2) * nfree */
    int num_fixed_re = (nfixed > 0) ? nfixed * nfree_verts : 0;
    int num_free_re = (nfixed + 1) * (nfree_verts / 2) * nfree;
    int num_re = num_fixed_re + num_free_re;

    /* Parse optional parameters */
    long long weight_period = (argc > 5) ? atoll(argv[5]) : (long long)num_re * 10;
    int weight_increment = (argc > 6) ? atoi(argv[6]) : 1;
    long long restart_period = (argc > 7) ? atoll(argv[7]) : (long long)num_re * num_re;
    int weight_cap = (argc > 8) ? atoi(argv[8]) : 1000;

    /* Print configuration */
    time_t start_time = time(NULL);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&start_time));

    printf("Start time: %s\n", time_buf);
    printf("n=%d, p=%d, s_target=%lld, seed=%u\n", n, p, s_target, seed);
    printf("nfixed=%d, nfree=%d\n", nfixed, nfree);
    printf("Original edges: %d, Reduced: num_rv=%d, num_re=%d\n", E_orig, num_rv, num_re);
    printf("Local range: ±%d, Search range: ±%d\n", LOCAL_RANGE, SEARCH_RANGE);
    printf("Evenslice weight: %.2f\n", EVENSLICE_WEIGHT);
    printf("weight_period=%lld, weight_increment=%d, restart_period=%lld, weight_cap=%d\n",
           weight_period, weight_increment, restart_period, weight_cap);

    /* Allocate arrays */
    int* M = malloc(sizeof(int) * p * (n + 1));      /* Coefficient matrix */
    int* best_M = malloc(sizeof(int) * p * (n + 1)); /* Best solution found */
    int* re_from = malloc(sizeof(int) * num_re);     /* Reduced edge endpoints */
    int* re_to = malloc(sizeof(int) * num_re);
    long long* re_mult = malloc(sizeof(long long) * num_re);          /* Edge multiplicities */
    long long* weights = malloc(sizeof(long long) * num_re);          /* Dynamic edge weights */
    unsigned char* splits_count = malloc(num_re);                     /* How many planes slice each edge */
    int* val = malloc(sizeof(int) * p * num_rv);                      /* Dot products */
    unsigned char* side = malloc(sizeof(unsigned char) * p * num_rv); /* Side indicators */
    unsigned char* new_side = malloc(sizeof(unsigned char) * num_rv); /* Temp for move eval */
    int8_t* diff = malloc(sizeof(int8_t) * num_re);                   /* Change in splits per edge */
    int* plane_slice_count = malloc(sizeof(int) * p);                 /* Edges sliced per plane */
    char* free_xsign = malloc(sizeof(char) * nfree_verts * nfree);    /* Precomputed signs */

    if (!M || !best_M || !re_from || !re_to || !re_mult || !weights ||
        !splits_count || !val || !side || !new_side || !diff ||
        !plane_slice_count || !free_xsign || !binom) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    /* Precompute sign of each free coordinate for each bit pattern.
     * free_xsign[fb * nfree + j] = +1 if bit j of fb is set, else -1 */
    for (int fb = 0; fb < nfree_verts; fb++) {
        for (int j = 0; j < nfree; j++) {
            free_xsign[fb * nfree + j] = ((fb >> j) & 1) ? +1 : -1;
        }
    }

    /* Build reduced edge list */
    int e = 0;

    /* Fixed edges: connect (pc, fb) to (pc+1, fb) for pc = 0..nfixed-1 */
    for (int pc = 0; pc < nfixed; pc++) {
        long long mult = binom[pc] * (nfixed - pc);
        for (int fb = 0; fb < nfree_verts; fb++) {
            int rv1 = pc * nfree_verts + fb;
            int rv2 = (pc + 1) * nfree_verts + fb;
            re_from[e] = rv1;
            re_to[e] = rv2;
            re_mult[e] = mult;
            e++;
        }
    }

    /* Free edges: connect vertices differing in one free coordinate */
    for (int pc = 0; pc <= nfixed; pc++) {
        long long mult = binom[pc];
        for (int fb = 0; fb < nfree_verts; fb++) {
            for (int d = 0; d < nfree; d++) {
                /* Only add edge once: when bit d is 0 in fb */
                if (!((fb >> d) & 1)) {
                    int rv1 = pc * nfree_verts + fb;
                    int rv2 = pc * nfree_verts + (fb ^ (1 << d));
                    re_from[e] = rv1;
                    re_to[e] = rv2;
                    re_mult[e] = mult;
                    e++;
                }
            }
        }
    }

    /* Verify edge count and multiplicity sum */
    if (e != num_re) {
        fprintf(stderr, "Edge count mismatch: %d vs expected %d\n", e, num_re);
        return 1;
    }

    long long total_mult = 0;
    for (e = 0; e < num_re; e++) {
        total_mult += re_mult[e];
    }
    if (total_mult != E_orig) {
        fprintf(stderr, "Multiplicity sum %lld != original edges %d\n", total_mult, E_orig);
        return 1;
    }
    printf("Multiplicity check OK: sum=%lld\n", total_mult);

    /* Initialize search state */
    long long W_current, S_current;
    int R_current;
    init_random_state(n, p, nfixed, nfree, num_rv, num_re,
                      re_from, re_to, re_mult, M, weights, free_xsign,
                      splits_count, val, side, plane_slice_count,
                      &W_current, &S_current, &R_current);

    long long best_S = S_current;
    int best_R = R_current;
    memcpy(best_M, M, sizeof(int) * p * (n + 1));

    printf("Initial: S=%lld/%lld, R=%d/%d\n", S_current, s_target, R_current, num_re);

    if (S_current >= s_target) {
        print_solution(n, p, M);
        return 0;
    }

    /* Main search loop */
    long long no_improve_W = 0; /* Iterations since W improved */
    long long no_improve_S = 0; /* Iterations since S improved */
    long long total_iters = 0;
    long long report_interval = (long long)num_re * 10000;
    int num_restarts = 0;

    while (1) {
        total_iters++;

        /* Periodic progress report */
        if (total_iters % report_interval == 0) {
            printf("iter=%lld, S=%lld/%lld, R=%d/%d, best_S=%lld, best_R=%d, W=%lld, restarts=%d\n",
                   total_iters, S_current, s_target, R_current, num_re,
                   best_S, best_R, W_current, num_restarts);
        }

        /* Select random hyperplane and two distinct free coordinates */
        int i = rand() % p;
        int j_free1 = rand() % nfree;
        int j_free2;
        do {
            j_free2 = rand() % nfree;
        } while (j_free2 == j_free1);

        int j1 = nfixed + j_free1;
        int j2 = nfixed + j_free2;

        int* val_i = val + i * num_rv;
        unsigned char* side_i = side + i * num_rv;

        /* Current coefficient values */
        int old1 = M[i * (n + 1) + j1];
        int old2 = M[i * (n + 1) + j2];

        /* Generate new values within local neighborhood */
        int lo1 = (old1 - LOCAL_RANGE < -SEARCH_RANGE) ? -SEARCH_RANGE : old1 - LOCAL_RANGE;
        int hi1 = (old1 + LOCAL_RANGE > SEARCH_RANGE) ? SEARCH_RANGE : old1 + LOCAL_RANGE;
        int new1;
        do {
            new1 = lo1 + rand() % (hi1 - lo1 + 1);
        } while (new1 == old1);

        int lo2 = (old2 - LOCAL_RANGE < -SEARCH_RANGE) ? -SEARCH_RANGE : old2 - LOCAL_RANGE;
        int hi2 = (old2 + LOCAL_RANGE > SEARCH_RANGE) ? SEARCH_RANGE : old2 + LOCAL_RANGE;
        int new2;
        do {
            new2 = lo2 + rand() % (hi2 - lo2 + 1);
        } while (new2 == old2);

        int delta1 = new1 - old1;
        int delta2 = new2 - old2;

        /* Compute new side values for all reduced vertices of this hyperplane */
        for (int pc = 0; pc <= nfixed; pc++) {
            for (int fb = 0; fb < nfree_verts; fb++) {
                int rv = pc * nfree_verts + fb;
                int new_val = val_i[rv] + delta1 * free_xsign[fb * nfree + j_free1] + delta2 * free_xsign[fb * nfree + j_free2];
                new_side[rv] = compute_side(new_val);
            }
        }

        /* Evaluate move: compute changes to W, S, and per-plane slice count */
        long long deltaW = 0, deltaS = 0;
        int delta_plane_count = 0;

        for (int e = 0; e < num_re; e++) {
            int rv1 = re_from[e], rv2 = re_to[e];
            int old_split = (side_i[rv1] != side_i[rv2]);
            int new_split = (new_side[rv1] != new_side[rv2]);
            int8_t d = (int8_t)(new_split - old_split);
            diff[e] = d;

            if (d == 1) {
                /* This plane now slices this edge */
                delta_plane_count++;
                if (splits_count[e] == 0) {
                    /* Edge was unsliced, now sliced */
                    deltaW += weights[e];
                    deltaS += re_mult[e];
                }
            } else if (d == -1) {
                /* This plane no longer slices this edge */
                delta_plane_count--;
                if (splits_count[e] == 1) {
                    /* Edge was sliced only by this plane, now unsliced */
                    deltaW -= weights[e];
                    deltaS -= re_mult[e];
                }
            }
        }

        /* Compute evenness penalty change (variance of slice counts across planes).
         * Variance is proportional to Sum_i(c_i^2) - ((Sum_i c_i)^2)/p
         * We only need the change in this quantity for acceptance. */
        int old_count = plane_slice_count[i];
        int new_count = old_count + delta_plane_count;

        long long old_sum_sq = 0, old_total = 0;
        for (int pi = 0; pi < p; pi++) {
            old_sum_sq += (long long)plane_slice_count[pi] * plane_slice_count[pi];
            old_total += plane_slice_count[pi];
        }

        long long new_sum_sq = old_sum_sq + (long long)new_count * new_count - (long long)old_count * old_count;
        long long new_total = old_total + delta_plane_count;

        double old_penalty = (double)old_sum_sq - (double)old_total * old_total / p;
        double new_penalty = (double)new_sum_sq - (double)new_total * new_total / p;
        double delta_penalty = new_penalty - old_penalty;

        /* Adjusted score: maximize W while minimizing variance */
        double adjusted_deltaW = (double)deltaW - EVENSLICE_WEIGHT * delta_penalty;

        /* Acceptance criteria:
         * 1. Adjusted W improves, OR
         * 2. Adjusted W ties and S doesn't decrease, OR
         * 3. Move achieves the target (always accept) */
        int accept = (adjusted_deltaW > 0) || (adjusted_deltaW == 0 && deltaS >= 0) || (S_current + deltaS >= s_target);

        if (accept) {
            /* Apply the move */
            M[i * (n + 1) + j1] = new1;
            M[i * (n + 1) + j2] = new2;

            /* Update val and side for this hyperplane */
            for (int pc = 0; pc <= nfixed; pc++) {
                for (int fb = 0; fb < nfree_verts; fb++) {
                    int rv = pc * nfree_verts + fb;
                    val_i[rv] += delta1 * free_xsign[fb * nfree + j_free1] + delta2 * free_xsign[fb * nfree + j_free2];
                    side_i[rv] = new_side[rv];
                }
            }

            plane_slice_count[i] = new_count;

            /* Update splits_count and global statistics */
            for (int e = 0; e < num_re; e++) {
                int8_t d = diff[e];
                if (d != 0) {
                    int old_cnt = splits_count[e];
                    int new_cnt = old_cnt + d;
                    splits_count[e] = new_cnt;

                    if (old_cnt == 0 && new_cnt > 0) {
                        W_current += weights[e];
                        S_current += re_mult[e];
                        R_current++;
                    } else if (old_cnt > 0 && new_cnt == 0) {
                        W_current -= weights[e];
                        S_current -= re_mult[e];
                        R_current--;
                    }
                }
            }

            /* Track best solution */
            if (S_current > best_S) {
                best_S = S_current;
                best_R = (R_current > best_R) ? R_current : best_R;
                memcpy(best_M, M, sizeof(int) * p * (n + 1));
                printf("New best: S=%lld, R=%d at iter=%lld\n", best_S, best_R, total_iters);
                if (best_S < s_target) {
                    print_solution(n, p, M);
                }
            } else if (R_current > best_R) {
                best_R = R_current;
                printf("New best (reduced): S=%lld, R=%d at iter=%lld\n", best_S, best_R, total_iters);
                print_solution(n, p, M);
            }

            no_improve_W = (deltaW > 0) ? 0 : no_improve_W + 1;
            no_improve_S = (deltaS > 0) ? 0 : no_improve_S + 1;
        } else {
            no_improve_W++;
            no_improve_S++;
        }

        /* Check for success */
        if (S_current >= s_target) {
            print_solution(n, p, M);
            fflush(stdout);

            time_t end_time = time(NULL);
            long elapsed = (long)(end_time - start_time);
            printf("SUCCESS at iter=%lld, S=%lld/%lld, R=%d/%d, elapsed=%ld:%02ld:%02ld\n",
                   total_iters, best_S, (long long)E_orig, best_R, num_re,
                   elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);
            return 0;
        }

        /* Weight boosting: increase weights of unsliced edges to escape local optima */
        if (weight_period > 0 && no_improve_W >= weight_period) {
            for (int e = 0; e < num_re; e++) {
                if (splits_count[e] == 0) {
                    long long inc = UNWEIGHTED ? weight_increment : (long long)weight_increment * re_mult[e];
                    long long cap = UNWEIGHTED ? weight_cap : (long long)weight_cap * re_mult[e];
                    long long new_w = weights[e] + inc;
                    weights[e] = (new_w > cap) ? cap : new_w;
                }
            }
            no_improve_W = 0;
        }

        /* Random restart if stuck */
        if (restart_period > 0 && no_improve_S >= restart_period) {
            num_restarts++;
            printf("RESTART #%d at iter=%lld, S=%lld, R=%d, best_S=%lld, best_R=%d\n",
                   num_restarts, total_iters, S_current, R_current, best_S, best_R);

            init_random_state(n, p, nfixed, nfree, num_rv, num_re,
                              re_from, re_to, re_mult, M, weights, free_xsign,
                              splits_count, val, side, plane_slice_count,
                              &W_current, &S_current, &R_current);
            no_improve_W = no_improve_S = 0;
        }
    }

    return 0;
}
