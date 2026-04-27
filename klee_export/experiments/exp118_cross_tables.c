// exp118: exp73 variant — two separate small tables, writes to both,
// then cross-references reads. Read from table A using index from
// table B, and vice versa. This creates deeper constraint interactions
// between the two sets of writes.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int coll_a(int x) { return x + 0x1A; }
__attribute__((noinline)) int coll_b(int x) { return x + 0x1B; }
__attribute__((noinline)) int hit_cross_ab(int x) { return x + 0x2A; }
__attribute__((noinline)) int miss_cross_ab(int x) { return x + 0x2B; }
__attribute__((noinline)) int hit_cross_ba(int x) { return x + 0x3A; }
__attribute__((noinline)) int miss_cross_ba(int x) { return x + 0x3B; }
__attribute__((noinline)) int both_hit(int x) { return x + 0x4C; }
__attribute__((noinline)) int both_miss(int x) { return x + 0x4D; }

int main() {
    uint8_t wa[3], wb[3], cross[2];
    klee_make_symbolic(wa, sizeof(wa), "wa");
    klee_make_symbolic(wb, sizeof(wb), "wb");
    klee_make_symbolic(cross, sizeof(cross), "cross");

    int tableA[8] = {0};
    int tableB[8] = {0};
    int result = 0;

    // Write to table A (3 writes)
    for (int i = 0; i < 3; i++) {
        uint8_t idx = wa[i] & 0x07;
        if (tableA[idx] != 0) result = coll_a(result);
        tableA[idx] = i + 1;
    }

    // Write to table B (3 writes)
    for (int i = 0; i < 3; i++) {
        uint8_t idx = wb[i] & 0x07;
        if (tableB[idx] != 0) result = coll_b(result);
        tableB[idx] = i + 1;
    }

    // Cross-reference: read tableA at index from cross[0],
    // read tableB at index from cross[1]
    uint8_t ca = cross[0] & 0x07;
    uint8_t cb = cross[1] & 0x07;

    int va = tableA[ca];
    int vb = tableB[cb];

    if (va != 0 && vb != 0) {
        result = both_hit(result);
    } else if (va == 0 && vb == 0) {
        result = both_miss(result);
    } else if (va != 0) {
        result = hit_cross_ab(result);
    } else {
        result = hit_cross_ba(result);
    }

    return result;
}
