/*
 * Structural Unit 3: Recursive Tree Processing
 * Source pattern: readelf/d_print_comp_inner (401 BBs), bison/derivation_*,
 *                 flvmeta/amf_data_free, gnumake/free_a_tree, bc/free_a_tree
 * CFG shape: Recursive descent over a symbolic tree structure.
 *            Branch on node type → process children recursively.
 * Rule prediction: R8 → dfs (recursion → DFS natural fit, matches call stack)
 */
#include <klee/klee.h>

#define MAX_NODES 15

struct node {
    unsigned char type;   /* 0=leaf, 1=unary, 2=binary, 3=ternary */
    unsigned char value;
    int left;    /* index or -1 */
    int right;   /* index or -1 */
    int extra;   /* index or -1 (for ternary) */
};

struct node tree[MAX_NODES];
int result;

int process_tree(int idx, int depth) {
    if (idx < 0 || idx >= MAX_NODES || depth > 6)
        return 0;

    struct node *n = &tree[idx];
    int val = 0;

    switch (n->type & 3) {
        case 0: /* leaf */
            val = n->value;
            if (val > 200) result |= 1;
            if (val < 50)  result |= 2;
            break;
        case 1: /* unary */
            val = process_tree(n->left, depth + 1);
            if (n->value & 1)
                val = -val;
            else
                val = val + 1;
            if (val > 100) result |= 4;
            break;
        case 2: /* binary */
            {
                int l = process_tree(n->left, depth + 1);
                int r = process_tree(n->right, depth + 1);
                if (n->value & 1)
                    val = l + r;
                else
                    val = l - r;
                if (val == 0) result |= 8;
                if (val > 500) result |= 16;
            }
            break;
        case 3: /* ternary */
            {
                int a = process_tree(n->left, depth + 1);
                int b = process_tree(n->right, depth + 1);
                int c = process_tree(n->extra, depth + 1);
                if (n->value & 1)
                    val = a + b + c;
                else if (n->value & 2)
                    val = a * 2 + b - c;
                else
                    val = a - b + c * 2;
                if (val == 42) result |= 32;
            }
            break;
    }
    return val;
}

int main() {
    klee_make_symbolic(tree, sizeof(tree), "tree");
    result = 0;
    int r = process_tree(0, 0);
    return result + (r & 0xFF);
}
