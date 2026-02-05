/*
  Hypercube Splitting HS(n,p,s) via Matrix‐Completion Heuristic
  Usage: ./hs_builder n p s seed [freeze_count] [coef_range]
    n            – dimension of hypercube (<20)
    p            – number of hyperplanes (<=n)
    s            – required number of split edges (<=n*2^(n-1))
    seed         – random seed (integer, ignored except for srand)
    freeze_count – (optional) how many coords to freeze per row [1..20], default=3
    coef_range   – (optional) absolute bound on filled coefficients [1..20], default=3

  The program runs until it finds an HS(n,p,s), then prints p lines of (n+1) ints.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

/* globals */
static int  n, p;
static long long s_target;
static int  freeze_count, coef_range;

/* edge list */
static int    Ecount;
static int   *edge_v;   /* one endpoint index */
static uint8_t *edge_j; /* which coordinate flips */

/* unsplit bitset */
static uint64_t *unsplit_mask;
static int       mask_words;
static int       unsplit_count;

/* solution matrix M[p][n+1] */
static int **M;
/* assigned flags per row (which positions are frozen) */
static uint8_t **assigned;

/* ---- bitset helpers ---- */
static inline int  bit_test(int e) {
    return (unsplit_mask[e >> 6] >> (e & 63)) & 1;
}
static inline void bit_clear(int e) {
    unsplit_mask[e >> 6] &= ~(1ULL << (e & 63));
}

/* ---- initialize edges of the n-cube ---- */
static void init_edges() {
    int v, j;
    int maxE = n * (1 << (n - 1));
    edge_v = malloc(sizeof(int) * maxE);
    edge_j = malloc(sizeof(uint8_t) * maxE);
    Ecount = 0;
    for (v = 0; v < (1 << n); v++) {
        for (j = 0; j < n; j++) {
            if ((v & (1 << j)) == 0) {
                /* edge between v and v|(1<<j) */
                edge_v[Ecount] = v;
                edge_j[Ecount] = (uint8_t) j;
                Ecount++;
            }
        }
    }
    /* allocate unsplit bitset */
    mask_words = (Ecount + 63) >> 6;
    unsplit_mask = calloc(mask_words, sizeof(uint64_t));
}

/* ---- reset all edges to unsplit ---- */
static void reset_unsplit() {
    for (int w = 0; w < mask_words; w++)
        unsplit_mask[w] = ~0ULL;
    /* clear extra bits beyond Ecount */
    if ((Ecount & 63) != 0) {
        uint64_t mask = (1ULL << (Ecount & 63)) - 1;
        unsplit_mask[mask_words - 1] = mask;
    }
    unsplit_count = Ecount;
}

/* ---- does row 'r' split the edge (v <-> v^(1<<d))? ---- */
static int row_splits(int r, int v, int d) {
    /* compute D1 = M[r]·x + bias, then D2 = D1 - 2*M[r][d]*x_d */
    int D1 = M[r][n];
    /* dot-product */
    for (int k = 0; k < n; k++) {
        int xk = ((v >> k) & 1) ? +1 : -1;
        D1 += M[r][k] * xk;
    }
    int xd = ((v >> d) & 1) ? +1 : -1;
    int D2 = D1 - 2 * M[r][d] * xd;
    /* sign positive if >= 1, negative if <= 0 */
    int s1 = (D1 >= 1);
    int s2 = (D2 >= 1);
    return (s1 != s2);
}

/* ---- count how many unsplit edges row 'r' would split ---- */
static int count_splits_row(int r) {
    int cnt = 0;
    for (int e = 0; e < Ecount; e++) {
        if (bit_test(e)) {
            if (row_splits(r, edge_v[e], edge_j[e]))
                cnt++;
        }
    }
    return cnt;
}

/* ---- update global unsplit set by removing edges split by row 'r' ---- */
static void update_unsplit(int r) {
    for (int e = 0; e < Ecount; e++) {
        if (bit_test(e)) {
            if (row_splits(r, edge_v[e], edge_j[e])) {
                bit_clear(e);
                unsplit_count--;
            }
        }
    }
}

/* ---- initialize partial guess for row 'r' by freezing top coords ---- */
static void init_partial_guess(int r) {
    struct Cand { int j, s, score; };
    struct Cand *cands = malloc(sizeof(*cands) * 2 * n);
    /* zero out the row */
    for (int j = 0; j <= n; j++) {
        M[r][j] = 0;
        assigned[r][j] = 0;
    }
    /* score each (j,±1) on current unsplit */
    int idx = 0;
    for (int j = 0; j < n; j++) {
        for (int sgn = -1; sgn <= +1; sgn += 2) {
            /* test hyperplane: only coeff j = sgn, bias=0 */
            M[r][j] = sgn;
            M[r][n] = 0;
            int sc = count_splits_row(r);
            cands[idx].j = j;
            cands[idx].s = sgn;
            cands[idx].score = sc;
            idx++;
            M[r][j] = 0;
        }
    }
    /* sort candidates desc by score (simple insertion sort) */
    for (int i = 1; i < 2*n; i++) {
        struct Cand tmp = cands[i];
        int k = i - 1;
        while (k >= 0 && cands[k].score < tmp.score) {
            cands[k+1] = cands[k];
            k--;
        }
        cands[k+1] = tmp;
    }
    /* pick top freeze_count distinct coordinates */
    int frozen = 0;
    for (int i = 0; i < 2*n && frozen < freeze_count; i++) {
        int j = cands[i].j;
        if (!assigned[r][j]) {
            assigned[r][j] = 1;
            M[r][j] = cands[i].s;
            frozen++;
        }
    }
    free(cands);
}

/* ---- complete row 'r': greedily fill unassigned positions, then refine ---- */
static void complete_row(int r) {
    int before, best_sc, best_c;
    /* initial greedy fill */
    for (int u = 0; u <= n; u++) {
        if (assigned[r][u]) continue;
        best_sc = -1;
        best_c  = 0;
        for (int c = -coef_range; c <= coef_range; c++) {
            M[r][u] = c;
            int sc = count_splits_row(r);
            if (sc > best_sc) {
                best_sc = sc;
                best_c  = c;
            }
        }
        M[r][u] = best_c;
    }
    /* local coordinate-wise improvements */
    int improvement = 1;
    while (improvement) {
        improvement = 0;
        for (int u = 0; u <= n; u++) {
            if (assigned[r][u]) continue;
            before = count_splits_row(r);
            int cur = M[r][u];
            best_sc = before;
            best_c  = cur;
            for (int c = -coef_range; c <= coef_range; c++) {
                if (c == cur) continue;
                M[r][u] = c;
                int sc = count_splits_row(r);
                if (sc > best_sc) {
                    best_sc = sc;
                    best_c  = c;
                }
            }
            if (best_c != cur) {
                M[r][u] = best_c;
                improvement = 1;
            } else {
                M[r][u] = cur;
            }
        }
    }
}

/* ---- main search loop ---- */
int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s n p s seed [freeze_count] [coef_range]\n", argv[0]);
        return 1;
    }
    n = atoi(argv[1]);
    p = atoi(argv[2]);
    s_target = atoll(argv[3]);
    int seed = atoi(argv[4]);
    srand(seed);

    /* hyperparams */
    freeze_count = (argc >= 6 ? atoi(argv[5]) : (n<3?n:3));
    if (freeze_count < 1 || freeze_count > n) freeze_count = (n<3?n:3);
    coef_range = (argc >= 7 ? atoi(argv[6]) : 3);
    if (coef_range < 1) coef_range = 3;
    if (coef_range > 20) coef_range = 20;

    /* allocate edges, M, assigned */
    init_edges();
    M = malloc(sizeof(int*) * p);
    assigned = malloc(sizeof(uint8_t*) * p);
    for (int i = 0; i < p; i++) {
        M[i] = calloc(n+1, sizeof(int));
        assigned[i] = calloc(n+1, sizeof(uint8_t));
    }

    /* main restart loop */
    while (1) {
        reset_unsplit();
        /* initial partial guesses */
        for (int i = 0; i < p; i++) {
            init_partial_guess(i);
        }
        /* iterative passes */
        int total_splits = 0;
        int stalled;
        do {
            stalled = 1;
            for (int i = 0; i < p; i++) {
                int before = unsplit_count;
                complete_row(i);
                update_unsplit(i);
                if (unsplit_count < before) stalled = 0;
                total_splits = Ecount - unsplit_count;
                if (total_splits >= s_target) {
                    /* found a valid solution */
                    for (int rr = 0; rr < p; rr++) {
                        for (int c = 0; c <= n; c++) {
                            printf("%d%c", M[rr][c], (c==n?'\n':' '));
                        }
                    }
                    return 0;
                }
            }
        } while (!stalled);
        /* if stalled, restart with new random partial guesses */
    }

    return 0;
}