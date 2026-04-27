// exp23: "Coverage vs Distance conflict" — Design a program where
// the coverage-novel path is FAR from uncovered code,
// and the close-to-uncovered path has NO coverage novelty.
// NURS:covnew should go one way, NURS:md2u the other.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int unique_fn_A(int x) { return x + 111; }
__attribute__((noinline)) int unique_fn_B(int x) { return x + 222; }
__attribute__((noinline)) int unique_fn_C(int x) { return x + 333; }
__attribute__((noinline)) int unique_fn_D(int x) { return x + 444; }
__attribute__((noinline)) int target_fn(int x)   { return x + 999; }

int main() {
    uint8_t route;
    uint8_t data[4];
    klee_make_symbolic(&route, sizeof(route), "route");
    klee_make_symbolic(data, sizeof(data), "data");

    int r = 0;

    if (route < 85) {
        // ROUTE 1: "Coverage novelty" path — calls 4 unique functions
        // but target_fn is 20 branches deep behind a bitfield wall
        r = unique_fn_A(r);
        r = unique_fn_B(r);
        r = unique_fn_C(r);
        r = unique_fn_D(r);
        
        // After the novelty, a wall of bitfield branches before target
        for (int i = 0; i < 4; i++) {
            if (data[i] & 0x01) r++;
            if (data[i] & 0x02) r++;
            if (data[i] & 0x04) r++;
            if (data[i] & 0x08) r++;
            if (data[i] & 0x10) r++;
        }
        
        if (r == 42) return target_fn(r);
        return r;
        
    } else if (route < 170) {
        // ROUTE 2: "Close to target" path — no unique functions,
        // but target_fn is just 1 branch away
        if (data[0] == 0x77) return target_fn(data[0]);
        return 0;
        
    } else {
        // ROUTE 3: Filler
        return data[0] + data[1];
    }
}
