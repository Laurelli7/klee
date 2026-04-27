/*
 * Structural Unit 14: Diamond-Heavy Validation Chain
 * Source: gnumake/pattern_search (163 br, diamond_heavy + range_check),
 *         flvmeta/check_flv_file (319 icmp, diamond_heavy + serialization),
 *         tiffinfo/TIFFFetchNormalTag (274 br, diamond_heavy)
 *         Pattern in 8/8 programs (135 functions total)
 * CFG: Chain of if-then-else diamonds where EACH diamond reconverges
 *      before the next test. Each test may set a flag or modify state,
 *      but execution always continues to the next diamond.
 *      Different from SU02 (if-else with early returns) — here paths merge.
 * Hypothesis: Diamonds reconverge → every path reaches every subsequent
 *   test → constraint growth (R5) + convergent-divergent (R6).
 *   NURS should detect divergence after each merge point.
 * Prediction: nurs:covnew (R6: convergent-divergent pattern)
 */
#include <klee/klee.h>

int main() {
    unsigned char buf[12];
    klee_make_symbolic(buf, sizeof(buf), "buf");

    int flags = 0;
    int warnings = 0;
    int state = 0;

    /* Diamond 1: type check — both sides reconverge */
    if (buf[0] > 128)
        state += 10;
    else
        state += 20;
    /* reconverge */

    /* Diamond 2 */
    if (buf[1] & 0x01)
        flags |= 1;
    else
        warnings++;
    /* reconverge */

    /* Diamond 3 */
    if (buf[1] & 0x02)
        flags |= 2;
    else
        warnings++;

    /* Diamond 4 */
    if (buf[1] & 0x04) {
        flags |= 4;
        state += 5;
    } else {
        warnings++;
        state -= 5;
    }

    /* Diamond 5 */
    if (buf[2] < buf[3])
        state = state * 2;
    else
        state = state + 100;

    /* Diamond 6 — depends on accumulated state */
    if (state > 50) {
        if (buf[4] == 0xFF)
            flags |= 8;
        else
            warnings += 2;
    } else {
        if (buf[4] == 0x00)
            flags |= 16;
        else
            warnings += 3;
    }

    /* Diamond 7 */
    if (buf[5] >= 'A' && buf[5] <= 'Z')
        state += buf[5] - 'A';
    else if (buf[5] >= 'a' && buf[5] <= 'z')
        state += buf[5] - 'a' + 26;
    else
        state -= 10;

    /* Diamond 8 — multi-condition */
    if (buf[6] > 100 && buf[7] > 100)
        flags |= 32;
    else if (buf[6] > 100 || buf[7] > 100)
        flags |= 64;
    else
        flags |= 128;

    /* Diamond 9 */
    if (flags & warnings) {
        state += 1000;
        if (buf[8] == buf[9])
            state += 500;
    } else {
        state -= 1000;
        if (buf[8] != buf[9])
            state -= 500;
    }

    /* Diamond 10 — final cross-check using accumulated state */
    if (state > 0 && flags > 0 && warnings < 5) {
        if (buf[10] + buf[11] > 200)
            return state + flags;
        else
            return state - flags;
    } else if (state < 0) {
        return -state + warnings;
    } else {
        return flags * warnings;
    }
}
