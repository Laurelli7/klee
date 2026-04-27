/*
 * Structural Unit 09: Cascaded Switches with Grouped Cases
 * Source: readelf cascaded_switches + grouped_switch_cases
 *         (e.g., relocation type → grouped handlers)
 * CFG: Multiple switches in sequence, many cases map to same handler.
 *      Different from SU01 (single flat switch) — here the switches are
 *      dependent: switch1 result affects switch2 input.
 * Hypothesis: The grouped cases reduce effective fan-out, making it more
 *   like a deep chain than a wide tree → DFS or covnew may beat RP
 * Prediction: nurs:covnew (R4 subcase: fewer effective branches + coverage signal)
 */
#include <klee/klee.h>

int classify_type(unsigned char t) {
    switch (t) {
        case 0: case 1: case 2: case 3:    return 0;  /* group A */
        case 4: case 5: case 6: case 7:    return 1;  /* group B */
        case 8: case 9: case 10: case 11:  return 2;  /* group C */
        case 12: case 13: case 14: case 15: return 3;  /* group D */
        case 16: case 17: case 18: case 19: return 4;  /* group E */
        default: return 5;
    }
}

int process_by_class(int cls, unsigned char val) {
    switch (cls) {
        case 0:  /* group A: accumulate */
            return val + (val >> 2);
        case 1:  /* group B: shift pattern */
            return (val << 1) ^ (val >> 3);
        case 2:  /* group C: range check chain */
            if (val < 50) return 10;
            else if (val < 100) return 20;
            else if (val < 150) return 30;
            else if (val < 200) return 40;
            else return 50;
        case 3:  /* group D: bitwise mask */
            return (val & 0x0F) * (val >> 4);
        case 4:  /* group E: nested condition */
            if (val & 1) {
                if (val & 2) return val;
                else return val ^ 0xFF;
            } else {
                if (val & 4) return val + 100;
                else return val - 100;
            }
        default: return -1;
    }
}

int apply_modifier(int cls, int result, unsigned char mod) {
    switch (mod & 7) {
        case 0: return result;
        case 1: return result + 10;
        case 2: return result - 10;
        case 3: return result * 2;
        case 4: return result ^ 0xAA;
        case 5: return -result;
        case 6: return result & 0xFF;
        case 7: return result | 0x100;
    }
    return result;
}

int main() {
    unsigned char input[10];
    klee_make_symbolic(input, sizeof(input), "input");

    int total = 0;
    for (int i = 0; i < 5; i++) {
        int cls = classify_type(input[i * 2]);
        int val = process_by_class(cls, input[i * 2 + 1]);
        total += apply_modifier(cls, val, input[i * 2] ^ input[i * 2 + 1]);
    }
    return total;
}
