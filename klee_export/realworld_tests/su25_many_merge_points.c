/*
 * Structural Unit 25: Many Merge Points
 * Source: many_merge_points (117 functions across ALL 8 programs)
 *         Functions with high fan-in at multiple points. E.g., error handling
 *         paths that all converge, gnumake variable resolution paths,
 *         lua type coercion paths that reconverge.
 * CFG: Multiple divergence points, but each one reconverges quickly.
 *      Unlike diamond_heavy (SU14) where single if-then-else reconverges,
 *      here the merge points come from multiple different sources (like
 *      error cleanup, or alternative computation paths).
 * Hypothesis: Many merge points = many states share the same continuation.
 *   This should help state merging and coverage. covnew should navigate well.
 * Prediction: nurs:covnew (merge points reduce effective state space)
 */
#include <klee/klee.h>

int process_record(unsigned char *data) {
    int status = 0;
    int value = 0;

    /* Phase 1: Parse header — multiple paths merge into value */
    if (data[0] == 0x01) {
        value = data[1];
    } else if (data[0] == 0x02) {
        value = data[1] | (data[2] << 8);
    } else if (data[0] == 0x03) {
        value = data[1] + data[2] + data[3];
    } else {
        value = data[0];  /* fallback */
    }
    /* merge point 1: all paths produced 'value' */

    /* Phase 2: Validate — multiple error paths merge into status */
    if (value < 0) {
        status = -1;
    } else if (value > 1000) {
        status = -2;
    } else if (value == 0 && data[0] != 0x01) {
        status = -3;
    } else {
        status = 1;
    }
    /* merge point 2: all paths produced 'status' */

    if (status < 0) {
        /* Error handling — multiple error codes merge here */
        if (data[4] & 0x80) {
            return status - 100;  /* critical error */
        }
        return status;
    }
    /* merge point 3: only success paths continue */

    /* Phase 3: Transform — three alternative algorithms, all produce result */
    int result;
    if (data[4] < 85) {
        /* Algorithm A: additive */
        result = value + data[5] + data[6];
    } else if (data[4] < 170) {
        /* Algorithm B: multiplicative */
        result = value * (data[5] + 1);
        if (result > 60000) result = 60000;
    } else {
        /* Algorithm C: bitwise */
        result = value ^ (data[5] << 4) ^ (data[6] >> 2);
    }
    /* merge point 4: all algorithms produced 'result' */

    /* Phase 4: Post-process — conditional adjustments, all merge */
    if (data[7] & 0x01) result += 10;
    /* merge point 5 */
    if (data[7] & 0x02) result -= 5;
    /* merge point 6 */
    if (data[7] & 0x04) result *= 2;
    /* merge point 7 */
    if (data[7] & 0x08) result = result >> 1;
    /* merge point 8 */

    /* Phase 5: Final classification — multiple paths to output */
    if (result > 500) {
        if (data[0] == 0x03) return 100;
        return 50;
    } else if (result > 100) {
        if (status == 1 && value > 50) return 30;
        return 20;
    } else if (result > 0) {
        return 10;
    } else {
        return 0;
    }
    /* merge point 9: function return */
}

int main() {
    unsigned char data[8];
    klee_make_symbolic(data, sizeof(data), "data");
    return process_record(data);
}
