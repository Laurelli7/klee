/*
 * Structural Unit 4: Byte-Processing State Machine
 * Source pattern: tiffinfo/Fax3Decode2D (744 BBs), readelf/inflate (601 BBs),
 *                 all programs doing binary format parsing byte-by-byte
 * CFG shape: Outer loop reads bytes, inner switch/branch on state,
 *            transitions produce output. Loop + dispatch combination.
 * Rule prediction: R9/R10 → nurs:covnew (byte loop with state machine,
 *                  coverage-guided explores new states efficiently)
 */
#include <klee/klee.h>

#define INPUT_LEN 12

enum state { S_INIT, S_HEADER, S_LENGTH, S_DATA, S_ESCAPE, S_CHECKSUM, S_DONE, S_ERROR };

int decode_stream(unsigned char *input, int len) {
    enum state st = S_INIT;
    int output = 0;
    int data_len = 0;
    int data_count = 0;
    int checksum = 0;
    int computed_check = 0;

    for (int i = 0; i < len && st != S_DONE && st != S_ERROR; i++) {
        unsigned char b = input[i];
        computed_check ^= b;

        switch (st) {
            case S_INIT:
                if (b == 0xAA) {
                    st = S_HEADER;
                } else if (b == 0xFF) {
                    st = S_ERROR; /* bad sync */
                }
                /* else stay in INIT (skip garbage) */
                break;

            case S_HEADER:
                if (b == 0x01 || b == 0x02 || b == 0x03) {
                    st = S_LENGTH;
                    output |= (b << 24);
                } else {
                    st = S_ERROR;
                }
                break;

            case S_LENGTH:
                data_len = b;
                if (data_len == 0) {
                    st = S_CHECKSUM;
                } else if (data_len > INPUT_LEN - 4) {
                    st = S_ERROR;
                } else {
                    data_count = 0;
                    st = S_DATA;
                }
                break;

            case S_DATA:
                if (b == 0x1B) {
                    st = S_ESCAPE;
                } else {
                    output ^= (b << ((data_count & 3) * 8));
                    data_count++;
                    if (data_count >= data_len) {
                        st = S_CHECKSUM;
                    }
                }
                break;

            case S_ESCAPE:
                /* Escape sequences */
                if (b == 0x00) {
                    output ^= (0x1B << ((data_count & 3) * 8)); /* literal escape */
                } else if (b == 0x01) {
                    output ^= (0xFF << ((data_count & 3) * 8)); /* special byte */
                } else if (b == 0x02) {
                    output ^= (0xAA << ((data_count & 3) * 8)); /* sync byte */
                } else {
                    st = S_ERROR;
                    break;
                }
                data_count++;
                if (data_count >= data_len)
                    st = S_CHECKSUM;
                else
                    st = S_DATA;
                break;

            case S_CHECKSUM:
                checksum = b;
                /* Verify: computed_check should be 0 if checksum matches */
                if ((computed_check ^ checksum) == checksum) {
                    st = S_DONE;
                    output |= 0x01; /* valid flag */
                } else {
                    st = S_ERROR;
                }
                break;

            default:
                break;
        }
    }

    if (st == S_DONE) return output;
    if (st == S_ERROR) return -1;
    return 0; /* incomplete */
}

int main() {
    unsigned char input[INPUT_LEN];
    klee_make_symbolic(input, sizeof(input), "input");
    return decode_stream(input, INPUT_LEN);
}
