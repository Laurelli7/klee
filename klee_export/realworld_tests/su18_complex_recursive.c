/*
 * Structural Unit 18: Complex Recursive with Internal Branching
 * Source: complex_recursive (26 functions across 5 programs)
 *         readelf/decode_location_expression (490 BBs, self-recursive!),
 *         bison/AnnotationList__computePredecessorAnnotations (114 BBs),
 *         gnumake/conditional_line (168 BBs)
 * CFG: Recursive function with a substantial body containing branches,
 *      switches, and multiple recursive call sites. Key difference from
 *      SU03 (simple recursive tree): the body itself has significant
 *      branching that creates an explosion at EACH recursion level.
 * Hypothesis: Body branches × recursion depth = double exponential.
 *   BFS may not help if body branches dominate. Coverage heuristics
 *   may detect that inner branches expose new code at each depth.
 * Prediction: nurs:covnew (body branches expose new code per depth + R8 variant)
 */
#include <klee/klee.h>

#define MAX_DEPTH 5

int process_node(unsigned char *data, int pos, int depth, int *result) {
    if (depth >= MAX_DEPTH || pos >= 16)
        return pos;

    unsigned char opcode = data[pos++];

    /* Complex body with significant internal branching */
    switch (opcode & 0x07) {
        case 0: /* literal value */
            *result += data[pos] - 128;
            pos++;
            break;
        case 1: /* unary op + recurse */
            pos = process_node(data, pos, depth + 1, result);
            *result = -*result;
            break;
        case 2: /* binary: recurse twice */
            {
                int left = 0, right = 0;
                pos = process_node(data, pos, depth + 1, &left);
                pos = process_node(data, pos, depth + 1, &right);
                if (opcode & 0x08)
                    *result = left + right;
                else
                    *result = left - right;
            }
            break;
        case 3: /* conditional: branch inside recursion */
            {
                int sub = 0;
                pos = process_node(data, pos, depth + 1, &sub);
                if (sub > 0) {
                    *result += sub * 2;
                    if (opcode & 0x10)
                        *result += 100;
                } else if (sub < 0) {
                    *result += sub;
                    if (opcode & 0x20)
                        *result -= 50;
                } else {
                    *result = 0;
                }
            }
            break;
        case 4: /* repeat: recurse N times */
            {
                int n = (opcode >> 3) & 3;
                for (int i = 0; i <= n && pos < 16; i++) {
                    int sub = 0;
                    pos = process_node(data, pos, depth + 1, &sub);
                    *result += sub;
                }
            }
            break;
        case 5: /* accumulate: recurse then combine */
            {
                int first = 0;
                pos = process_node(data, pos, depth + 1, &first);
                if (first > 50) {
                    int second = 0;
                    pos = process_node(data, pos, depth + 1, &second);
                    *result = first + second;
                } else {
                    *result = first;
                }
            }
            break;
        case 6: /* swap: two recursive then compare */
            {
                int a = 0, b = 0;
                pos = process_node(data, pos, depth + 1, &a);
                pos = process_node(data, pos, depth + 1, &b);
                if (a > b) *result = a;
                else *result = b;
                if (a == b) *result += 500;
            }
            break;
        case 7: /* terminal with flags */
            if (pos < 16) {
                unsigned char flags = data[pos++];
                if (flags & 1) *result += 10;
                if (flags & 2) *result += 20;
                if (flags & 4) *result += 40;
                if (flags & 8) *result += 80;
            }
            break;
    }

    return pos;
}

int main() {
    unsigned char input[16];
    klee_make_symbolic(input, sizeof(input), "input");

    int result = 0;
    process_node(input, 0, 0, &result);
    return result;
}
