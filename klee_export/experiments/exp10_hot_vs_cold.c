// exp10: "Hot loop" — an inner loop executes many times per path,
// creating a scenario where NURS:MD (min-dist-to-uncovered) and
// NURS:CovNew shine by steering away from already-covered loops.
#include "klee/klee.h"
int main() {
    unsigned char mode, data[4];
    klee_make_symbolic(&mode, sizeof(mode), "mode");
    klee_make_symbolic(data, sizeof(data), "data");

    if (mode < 10) {
        // Path A: Hot loop — computes but doesn't branch much
        int sum = 0;
        for (int i = 0; i < 4; i++) {
            sum += data[i];
        }
        if (sum == 0) return 1;
        if (sum > 500) return 2;
        return 3;
    } else if (mode < 20) {
        // Path B: Cold branchy code — lots of new coverage opportunity
        int r = data[0];
        if (data[1] & 0x01) r ^= 0x10;
        if (data[1] & 0x02) r ^= 0x20;
        if (data[1] & 0x04) r ^= 0x40;
        if (data[1] & 0x08) r ^= 0x80;
        if (data[2] > 128) r += 100;
        if (data[3] > 128) r += 200;
        if (r == 42) return -1; // BUG
        return r & 0xFF;
    } else {
        // Path C: No-op
        return 0;
    }
}
