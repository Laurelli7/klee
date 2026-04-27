// exp64: "Breadcrumb Trail" — A program where covnew should FAIL and
// md2u should WIN. The trick: scatter cheap "breadcrumb" coverage
// that covnew chases, while the REAL target (deep function) is at
// a measurable distance that md2u can track.
//
// Structure: 
// - Path A: 8 unique tiny functions (breadcrumbs), then DEAD END (return)
// - Path B: 8 shared (already-covered) steps, then a DEEP unique function
//
// covnew: will chase breadcrumbs (each is "new" coverage), reach dead end
// md2u: will see the deep function is N steps away, prefer Path B
// DFS: depends on fork ordering
// BFS: tries both paths, slow but covers both
//
// Between paths, noise branches amplify the state count.
#include "klee/klee.h"
#include <stdint.h>

// Breadcrumbs: unique but worthless (dead-end path)
__attribute__((noinline)) int crumb_0(int x) { return x | 0x001; }
__attribute__((noinline)) int crumb_1(int x) { return x | 0x002; }
__attribute__((noinline)) int crumb_2(int x) { return x | 0x004; }
__attribute__((noinline)) int crumb_3(int x) { return x | 0x008; }
__attribute__((noinline)) int crumb_4(int x) { return x | 0x010; }
__attribute__((noinline)) int crumb_5(int x) { return x | 0x020; }
__attribute__((noinline)) int crumb_6(int x) { return x | 0x040; }
__attribute__((noinline)) int crumb_7(int x) { return x | 0x080; }

// Shared steps: already-covered code (boring for covnew)
__attribute__((noinline)) int shared_step(int x, int i) { return x + i; }

// The real prize: deep, unique function
__attribute__((noinline)) int deep_prize(int x) { return x * 137 + 99999; }

int main() {
    uint8_t choice;
    uint8_t noise[3];
    klee_make_symbolic(&choice, sizeof(choice), "choice");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;

    // Noise: creates states that both paths must traverse
    for (int i = 0; i < 3; i++) {
        if (noise[i] & 0x01) result++;
        if (noise[i] & 0x02) result++;
        if (noise[i] & 0x04) result++;
        if (noise[i] & 0x08) result++;
        if (noise[i] & 0x10) result++;
        if (noise[i] & 0x20) result++;
        if (noise[i] & 0x40) result++;
        if (noise[i] & 0x80) result++;
    }

    if (choice < 128) {
        // Path A: Breadcrumb trail — lots of new coverage, dead end
        result = crumb_0(result);
        result = crumb_1(result);
        result = crumb_2(result);
        result = crumb_3(result);
        result = crumb_4(result);
        result = crumb_5(result);
        result = crumb_6(result);
        result = crumb_7(result);
        return result; // Dead end — no deep prize
    } else {
        // Path B: Boring shared steps, then deep prize
        for (int i = 0; i < 8; i++) {
            result = shared_step(result, i);
        }
        // Deep prize behind additional gate
        if ((choice & 0x0F) == 0x0A) {
            result = deep_prize(result);
        }
        return result;
    }
}
