// exp84: "Knotted CFG" — Irreducible control flow created by
// goto-connected loop nests. Two loops share a common body but
// have different entry points, creating a CFG that can't be
// reduced to a tree of loops.
//
// Loop A enters the shared body from condition CA.
// Loop B enters the shared body from condition CB.
// The shared body can exit to either loop's continuation.
// This means the same "shared body" code is reachable with
// fundamentally different constraint histories.
//
// Irreducible CFGs are rare in real code but appear in:
// - Duff's device patterns
// - Coroutine implementations
// - Hand-optimized state machines
// - Decompiled/obfuscated code
//
// random-path: tree model can't represent shared nodes
// DFS: may get stuck in one loop
// covnew: shared body looks "already covered" from second entry
// This should strongly differentiate random-path from all others.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int shared_body(int x, int pass) {
    return x + pass * 111;
}
__attribute__((noinline)) int loop_a_specific(int x) { return x + 0xA0; }
__attribute__((noinline)) int loop_b_specific(int x) { return x + 0xB0; }
__attribute__((noinline)) int exit_handler(int x) { return x + 9999; }

int main() {
    uint8_t ctrl[3];
    uint8_t noise;
    klee_make_symbolic(ctrl, sizeof(ctrl), "ctrl");
    klee_make_symbolic(&noise, sizeof(noise), "noise");

    int acc = 0;
    int pass = 0;

    // Noise
    if (noise & 0x01) acc++;
    if (noise & 0x02) acc++;
    if (noise & 0x04) acc++;
    if (noise & 0x08) acc++;
    if (noise & 0x10) acc++;
    if (noise & 0x20) acc++;
    if (noise & 0x40) acc++;
    if (noise & 0x80) acc++;

    // Entry: symbolic choice of which "loop" to enter
    if (ctrl[0] & 0x80)
        goto loop_b_entry;

loop_a_entry:
    acc = loop_a_specific(acc);
    if ((ctrl[0] & 0x03) == 0x01) goto shared;
    if ((ctrl[0] & 0x03) == 0x02) goto loop_b_entry;
    if ((ctrl[0] & 0x03) == 0x03) goto done;
    // fall through to shared
    goto shared;

loop_b_entry:
    acc = loop_b_specific(acc);
    if ((ctrl[1] & 0x03) == 0x01) goto shared;
    if ((ctrl[1] & 0x03) == 0x02) goto loop_a_entry;
    if ((ctrl[1] & 0x03) == 0x03) goto done;
    goto shared;

shared:
    pass++;
    acc = shared_body(acc, pass);
    if (pass >= 4) goto done;

    // Shared body dispatches BACK to either loop based on accumulated state
    if (ctrl[2] & (1 << (pass - 1))) {
        goto loop_a_entry;
    } else {
        goto loop_b_entry;
    }

done:
    acc = exit_handler(acc);

    if (pass == 4 && (acc & 0xFF) == 0x42) return 77777;
    return acc & 0xFFFF;
}
