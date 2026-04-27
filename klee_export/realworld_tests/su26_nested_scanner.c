/*
 * Structural Unit 26: Nested Scanner
 * Source: nested_scanner (64 functions across ALL 8 programs)
 *         Outer loop scans for delimiter/token boundary, inner loop scans
 *         within token. E.g., nasm tokenizer (line scanner + word scanner),
 *         lua string pattern matching, readelf string table processing.
 * CFG: Two nested loops, both with conditional exits. Outer loop advances
 *      position, inner loop processes current token. Inner loop may consume
 *      multiple bytes before returning control to outer loop.
 * Hypothesis: Nested scanners create deep paths. The inner loop's symbolic
 *   condition interacts with outer loop's position → state explosion.
 * Prediction: DFS (deep sequential nature + loop interactions favor DFS, R9)
 */
#include <klee/klee.h>

int main() {
    unsigned char input[10];
    klee_make_symbolic(input, sizeof(input), "input");

    int token_count = 0;
    int total_value = 0;
    int pos = 0;

    /* Outer scanner: find token starts */
    while (pos < 10) {
        /* Skip whitespace-like delimiters */
        while (pos < 10 && (input[pos] == 0x20 || input[pos] == 0x09 ||
                            input[pos] == 0x0A || input[pos] == 0x0D)) {
            pos++;
        }
        if (pos >= 10) break;

        /* Found token start */
        token_count++;
        int token_val = 0;
        int token_len = 0;

        /* Inner scanner: process token characters */
        if (input[pos] >= 0x30 && input[pos] <= 0x39) {
            /* Numeric token: scan digits */
            while (pos < 10 && input[pos] >= 0x30 && input[pos] <= 0x39) {
                token_val = token_val * 10 + (input[pos] - 0x30);
                if (token_val > 999) token_val = 999;  /* clamp */
                pos++;
                token_len++;
            }
            total_value += token_val;
        } else if (input[pos] >= 0x41 && input[pos] <= 0x5A) {
            /* Uppercase alpha token */
            while (pos < 10 && input[pos] >= 0x41 && input[pos] <= 0x5A) {
                token_val ^= input[pos];
                pos++;
                token_len++;
            }
            total_value += token_val + 100;
        } else if (input[pos] >= 0x61 && input[pos] <= 0x7A) {
            /* Lowercase alpha token */
            while (pos < 10 && input[pos] >= 0x61 && input[pos] <= 0x7A) {
                token_val += input[pos] - 0x60;
                pos++;
                token_len++;
            }
            total_value += token_val + 200;
        } else {
            /* Single-character special token */
            token_val = input[pos];
            pos++;
            token_len = 1;
            total_value += token_val + 300;
        }

        /* Token length affects scoring differently */
        if (token_len > 3) total_value *= 2;
    }

    /* Final classification */
    if (token_count == 0) return 0;
    if (token_count == 1) return 1;
    if (token_count >= 5) return 5;
    if (total_value > 1000) return 10;
    if (total_value > 500) return 7;
    return 3;
}
