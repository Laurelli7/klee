// exp33: "DFS traps itself" — A program where DFS deterministically
// picks the WORST path first at every branch, while BFS spreads out.
// Key: make the "true" side of each branch be a dead end,
// and the "false" side be the continuation. DFS takes true first.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int rare_code_1(int x) { return x + 1000; }
__attribute__((noinline)) int rare_code_2(int x) { return x + 2000; }
__attribute__((noinline)) int rare_code_3(int x) { return x + 3000; }

int main() {
    uint8_t x;
    uint8_t noise[5];
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;

    // Stage 1: Big bitfield wall when x < 200 (true branch)
    // Rare code only reachable when x >= 200 (false branch)
    if (x < 200) {
        // True: DFS enters here first. Wide bitfield.
        for (int i = 0; i < 5; i++) {
            if (noise[i] & 0x01) result++;
            if (noise[i] & 0x02) result++;
            if (noise[i] & 0x04) result++;
            if (noise[i] & 0x08) result++;
        }
        return result;
    }

    // False: Only reached when x >= 200
    result = rare_code_1(x);

    if (x < 230) {
        return result;
    }
    
    result = rare_code_2(result);
    
    if (x < 250) {
        return result;
    }
    
    result = rare_code_3(result);
    return result;
}
