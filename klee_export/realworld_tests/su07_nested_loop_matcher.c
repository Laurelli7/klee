/*
 * Structural Unit 7: Nested Loop Matcher
 * Source pattern: gnumake/pattern_search (395 BBs, 163 branches),
 *                 bison/AnnotationList__computePredecessorAnnotations (114 BBs, 41 br),
 *                 gnumake/update_file_1 (346 BBs), nasm/matches (150 BBs)
 * CFG shape: Outer loop iterates over candidates, inner loop checks conditions.
 *            Early exit on mismatch. Diamond-like reconvergence.
 * Rule prediction: R3/R5 → random-path (nested loops with diamonds,
 *                  random-path distributes exploration across loop iterations)
 */
#include <klee/klee.h>

#define N_PATTERNS 6
#define PAT_LEN 8
#define INPUT_LEN 10

/* Pattern matching — models gnumake's pattern_search */
int match_pattern(unsigned char *input, int ilen, unsigned char *pattern, int plen) {
    int score = 0;
    int ii = 0, pi = 0;

    while (ii < ilen && pi < plen) {
        if (pattern[pi] == '*') {
            /* Wildcard: skip ahead in input */
            pi++;
            if (pi >= plen) return score + (ilen - ii); /* rest matches */
            /* Find next match in input */
            int found = 0;
            while (ii < ilen) {
                if (input[ii] == pattern[pi]) {
                    found = 1;
                    break;
                }
                ii++;
                score++;
            }
            if (!found) return -1;
        } else if (pattern[pi] == '?') {
            /* Match any single character */
            score += 2;
            ii++;
            pi++;
        } else if (input[ii] == pattern[pi]) {
            /* Exact match */
            score += 3;
            ii++;
            pi++;
        } else {
            return -1; /* mismatch */
        }
    }

    if (pi < plen) return -1; /* pattern not fully matched */
    return score;
}

/* Models bison's annotation predecessor computation */
int compute_predecessors(unsigned char *states, int n, unsigned char target) {
    int count = 0;
    int best_distance = 999;

    for (int i = 0; i < n; i++) {
        /* Check if state i can reach target */
        int reachable = 0;
        int distance = 0;

        for (int j = i; j < n; j++) {
            distance++;
            if (states[j] == target) {
                reachable = 1;
                break;
            }
            /* Check intermediate conditions */
            if (states[j] > target + 10) break; /* unreachable */
            if (states[j] == 0) break; /* barrier */
        }

        if (reachable) {
            count++;
            if (distance < best_distance) {
                best_distance = distance;
            }
        }
    }

    return count * 100 + best_distance;
}

int main() {
    unsigned char input[INPUT_LEN];
    unsigned char patterns[N_PATTERNS][PAT_LEN];
    klee_make_symbolic(input, sizeof(input), "input");
    klee_make_symbolic(patterns, sizeof(patterns), "patterns");

    int best_score = -1;
    int best_pat = -1;

    /* Outer loop: try each pattern */
    for (int p = 0; p < N_PATTERNS; p++) {
        int score = match_pattern(input, INPUT_LEN, patterns[p], PAT_LEN);
        if (score > best_score) {
            best_score = score;
            best_pat = p;
        }
    }

    /* Also compute predecessor metric */
    int pred = compute_predecessors(input, INPUT_LEN, patterns[0][0]);

    return best_pat * 10000 + best_score * 100 + (pred & 0xFF);
}
