#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr,
            "Usage: %s n p s seed [weight_period] [weight_increment] [restart_period]\n",
            argv[0]);
        return 1;
    }
    int n     = atoi(argv[1]);           /* dimension */
    int p     = atoi(argv[2]);           /* number of hyperplanes */
    long long s_target = atoll(argv[3]); /* required split‐edge count */
    unsigned int seed = (unsigned int)atoll(argv[4]);

    /* optional hyperparameters */
    long long weight_period    = (argc > 5 ? atoll(argv[5]) : 10000LL);
    int       weight_increment = (argc > 6 ? atoi(argv[6]) : 1);
    long long restart_period   = (argc > 7 ? atoll(argv[7]) : 0LL);

    srand(seed);

    /* number of vertices and edges in the n-cube */
    int nverts = 1 << n;
    int E = n * (1 << (n - 1));  /* n * 2^(n-1) */

    /* allocate core arrays */
    int *M = malloc(sizeof(int) * p * (n + 1));           /* M[i*(n+1)+j] */
    char *xsign = malloc(sizeof(char) * nverts * n);      /* xsign[v*n + j] = ±1 */
    int *edges_v = malloc(sizeof(int) * E);
    int *edges_w = malloc(sizeof(int) * E);
    int *weights = malloc(sizeof(int) * E);
    unsigned char *splits_count = malloc(E);
    int *val = malloc(sizeof(int) * p * nverts);          /* val[i*nverts + v] */
    unsigned char *side = malloc(sizeof(unsigned char) * p * nverts);
    unsigned char *new_side = malloc(sizeof(unsigned char) * nverts);
    int8_t *diff = malloc(sizeof(int8_t) * E);

    if (!M || !xsign || !edges_v || !edges_w || !weights
        || !splits_count || !val || !side || !new_side || !diff) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    /* 1) Randomly initialize M in [−20..20] */
    for (int i = 0; i < p; i++) {
        for (int j = 0; j <= n; j++) {
            M[i*(n+1) + j] = (rand() % 41) - 20;
        }
    }

    /* 2) build xsign[v,j] = coordinate j of vertex v in {±1} */
    for (int v = 0; v < nverts; v++) {
        for (int j = 0; j < n; j++) {
            xsign[v*n + j] = ((v >> j) & 1) ? +1 : -1;
        }
    }

    /* 3) build edge list edges_v, edges_w (each undirected edge once) */
    int e = 0;
    for (int v = 0; v < nverts; v++) {
        for (int d = 0; d < n; d++) {
            if (!((v >> d) & 1)) {
                int w = v ^ (1 << d);
                edges_v[e] = v;
                edges_w[e] = w;
                e++;
            }
        }
    }
    /* sanity */
    if (e != E) {
        fprintf(stderr, "edge count mismatch %d vs %d\n", e, E);
        return 1;
    }

    /* 4) init weights and evaluate initial val, side */
    for (e = 0; e < E; e++) {
        weights[e] = 1;
    }
    for (int i = 0; i < p; i++) {
        for (int v = 0; v < nverts; v++) {
            int tot = M[i*(n+1) + n]; /* constant term */
            for (int j = 0; j < n; j++) {
                tot += M[i*(n+1) + j] * xsign[v*n + j];
            }
            val[i*nverts + v] = tot;
            side[i*nverts + v] = (tot >= 1) ? 1 : 0;
        }
    }

    /* 5) init splits_count, S_current, W_current */
    long long W_current = 0;
    long long S_current = 0;
    for (e = 0; e < E; e++) {
        int v = edges_v[e], wv = edges_w[e];
        int cnt = 0;
        for (int i = 0; i < p; i++) {
            cnt += (side[i*nverts + v] != side[i*nverts + wv]);
        }
        splits_count[e] = cnt;
        if (cnt > 0) {
            S_current++;
            W_current += weights[e];
        }
    }
    /* immediate success? */
    if (S_current >= s_target) {
        for (int i = 0; i < p; i++) {
            for (int j = 0; j <= n; j++) {
                printf("%d%c",
                    M[i*(n+1) + j],
                    j == n ? '\n' : ' ');
            }
        }
        return 0;
    }

    /* 6) main local‐search loop */
    long long no_imp_W = 0, no_imp_S = 0;
    while (1) {
        /* pick a random hyperplane‐row and a random column */
        int i = rand() % p;
        int j = rand() % (n + 1);
        int oldm = M[i*(n+1) + j];
        int newm;
        do {
            newm = (rand() % 41) - 20;
        } while (newm == oldm);
        int delta = newm - oldm;

        /* compute new_side[v] for this plane i only */
        if (j < n) {
            for (int v = 0; v < nverts; v++) {
                int nv = val[i*nverts + v] + delta * xsign[v*n + j];
                new_side[v] = (nv >= 1) ? 1 : 0;
            }
        } else {
            /* constant term changed */
            for (int v = 0; v < nverts; v++) {
                int nv = val[i*nverts + v] + delta;
                new_side[v] = (nv >= 1) ? 1 : 0;
            }
        }

        /* scan edges to compute deltaW, deltaS and record diffs */
        long long deltaW = 0;
        int deltaS_count = 0;
        for (e = 0; e < E; e++) {
            int v = edges_v[e], wv = edges_w[e];
            int old_diff = side[i*nverts + v] ^ side[i*nverts + wv];
            int new_diff = new_side[v]     ^ new_side[wv];
            int8_t d = (int8_t)(new_diff - old_diff);
            diff[e] = d;
            if (d == 1) {
                /* old_diff==0 -> new_diff==1 */
                if (splits_count[e] == 0) {
                    deltaW += weights[e];
                    deltaS_count++;
                }
            } else if (d == -1) {
                /* old_diff==1 -> new_diff==0 */
                if (splits_count[e] == 1) {
                    deltaW -= weights[e];
                    deltaS_count--;
                }
            }
        }

        /* greedy acceptance */
        if (deltaW >= 0) {
            /* commit the move */
            M[i*(n+1) + j] = newm;
            /* update val and side for plane i */
            if (j < n) {
                for (int v = 0; v < nverts; v++) {
                    val[i*nverts + v] += delta * xsign[v*n + j];
                    side[i*nverts + v] = new_side[v];
                }
            } else {
                for (int v = 0; v < nverts; v++) {
                    val[i*nverts + v] += delta;
                    side[i*nverts + v] = new_side[v];
                }
            }
            /* update splits_count, W_current, S_current */
            for (e = 0; e < E; e++) {
                int8_t d = diff[e];
                if (d != 0) {
                    int oc = splits_count[e];
                    int nc = oc + d;
                    splits_count[e] = nc;
                    if (oc == 0 && nc > 0) {
                        W_current += weights[e];
                        S_current++;
                    } else if (oc > 0 && nc == 0) {
                        W_current -= weights[e];
                        S_current--;
                    }
                }
            }
            /* reset/improve counters */
            no_imp_W = 0;
            if (deltaS_count > 0) {
                no_imp_S = 0;
            } else {
                no_imp_S++;
            }
        } else {
            /* reject: only bump no_imp_W */
            no_imp_W++;
            no_imp_S++;
        }

        /* found a valid splitting? */
        if (S_current >= s_target) {
            for (int ii = 0; ii < p; ii++) {
                for (int jj = 0; jj <= n; jj++) {
                    printf("%d%c",
                        M[ii*(n+1) + jj],
                        jj == n ? '\n' : ' ');
                }
            }
            fflush(stdout);
            return 0;
        }

        /* adaptive weight boosting */
        if (weight_period > 0 && no_imp_W >= weight_period) {
            for (e = 0; e < E; e++) {
                if (splits_count[e] == 0) {
                    weights[e] += weight_increment;
                }
            }
            no_imp_W = 0;
        }

        /* optional random restart */
        if (restart_period > 0 && no_imp_S >= restart_period) {
            /* re‐randomize M and recompute all data structures */
            for (int ii = 0; ii < p; ii++) {
                for (int jj = 0; jj <= n; jj++) {
                    M[ii*(n+1) + jj] = (rand() % 41) - 20;
                }
            }
            /* recompute val, side */
            for (int ii = 0; ii < p; ii++) {
                for (int v = 0; v < nverts; v++) {
                    int tot = M[ii*(n+1) + n];
                    for (int jj = 0; jj < n; jj++) {
                        tot += M[ii*(n+1) + jj] * xsign[v*n + jj];
                    }
                    val[ii*nverts + v] = tot;
                    side[ii*nverts + v] = (tot >= 1) ? 1 : 0;
                }
            }
            /* recompute splits_count, S_current, W_current */
            W_current = S_current = 0;
            for (e = 0; e < E; e++) {
                int v = edges_v[e], wv = edges_w[e];
                int cnt = 0;
                for (int ii = 0; ii < p; ii++) {
                    cnt += (side[ii*nverts + v] != side[ii*nverts + wv]);
                }
                splits_count[e] = cnt;
                if (cnt > 0) {
                    S_current++;
                    W_current += weights[e];
                }
            }
            no_imp_W = no_imp_S = 0;
        }
    }

    /* never reached */
    return 0;
}