/*
 * Structural Unit 20: Transform Loop
 * Source: transform_loop (395 functions across ALL 8 programs)
 *         The MOST common non-trivial pattern. E.g., lua string transforms,
 *         readelf section processing, bison grammar rewrites, bc value conversions.
 * CFG: Single loop where body applies conditional transformation to each element.
 *      Branch inside loop selects which transform to apply, but all paths
 *      rejoin at loop latch. Different from scanner_loop (SU04) which is
 *      about state transitions; this is about per-element data transformation.
 * Hypothesis: The loop creates depth, but the branch inside creates coverage
 *   targets. covnew should be attracted to uncovered transform branches.
 * Prediction: nurs:covnew (coverage-guided picks uncovered transforms)
 */
#include <klee/klee.h>

int main() {
    unsigned char data[12];
    klee_make_symbolic(data, sizeof(data), "data");

    int result = 0;

    /* Transform loop: apply per-element conditional transformation */
    for (int i = 0; i < 12; i++) {
        unsigned char val = data[i];
        unsigned char transformed;

        if (val < 32) {
            /* Control characters: escape */
            transformed = val + 64;
        } else if (val < 48) {
            /* Punctuation: normalize to space */
            transformed = 32;
        } else if (val < 58) {
            /* Digits: convert to value */
            transformed = val - 48;
            result += transformed;
        } else if (val < 65) {
            /* Symbols: hash */
            transformed = (val ^ 0x5A) & 0x3F;
        } else if (val < 91) {
            /* Uppercase: to lowercase */
            transformed = val + 32;
        } else if (val < 97) {
            /* Brackets etc: identity */
            transformed = val;
        } else if (val < 123) {
            /* Lowercase: to uppercase */
            transformed = val - 32;
        } else {
            /* High bytes: fold into ASCII range */
            transformed = val & 0x7F;
        }

        /* Accumulate transformed value differently by position */
        if (i < 4) {
            result ^= transformed;
        } else if (i < 8) {
            result += transformed;
        } else {
            result -= transformed;
        }
    }

    /* Final check on accumulated result */
    if (result == 42)
        return 1;
    else if (result == -17)
        return 2;
    else if (result > 200)
        return 3;
    else
        return 0;
}
