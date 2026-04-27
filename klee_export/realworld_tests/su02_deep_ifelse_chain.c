/*
 * Structural Unit 2: Deep If-Else Chain
 * Source pattern: gnumake/main (192 branches), flvmeta/check_flv_file (193),
 *                 bison/gram_lex (299 branches), all programs parsing/validation
 * CFG shape: Long sequential chain of conditional tests on symbolic input.
 *            Each condition is independent (not nested). Deep, narrow.
 * Rule prediction: R1/R2 → nurs:covnew (deep sequential, coverage-guided best)
 */
#include <klee/klee.h>

int warnings;
int errors;
int flags;

void warn(int code) { warnings |= (1 << (code & 15)); }
void err(int code) { errors |= (1 << (code & 15)); }
void set_flag(int bit) { flags |= (1 << (bit & 31)); }

int validate_config(unsigned char *buf, int len) {
    int result = 0;

    /* Sequential validation checks — models field-by-field config parsing */
    if (len < 4) { err(0); return -1; }

    /* Magic bytes */
    if (buf[0] != 0x89) { err(1); return -1; }
    if (buf[1] != 'P')  { warn(0); }
    if (buf[2] != 'N')  { warn(1); }
    if (buf[3] != 'G')  { warn(2); }

    /* Version field */
    if (buf[4] < 1 || buf[4] > 5) { err(2); return -2; }
    set_flag(0);

    /* Flags byte */
    if (buf[5] & 0x01) set_flag(1);
    if (buf[5] & 0x02) set_flag(2);
    if (buf[5] & 0x04) set_flag(3);
    if (buf[5] & 0x08) set_flag(4);
    if (buf[5] & 0x10) set_flag(5);
    if (buf[5] & 0x20) set_flag(6);
    if (buf[5] & 0x40) set_flag(7);
    if (buf[5] & 0x80) set_flag(8);

    /* Type validation */
    if (buf[6] == 0) {
        result = 1;
        if (buf[7] > 100) warn(3);
        if (buf[7] < 10) warn(4);
    } else if (buf[6] == 1) {
        result = 2;
        if (buf[7] != buf[8]) warn(5);
    } else if (buf[6] == 2) {
        result = 3;
        if (buf[7] + buf[8] > 200) warn(6);
    } else if (buf[6] == 3) {
        result = 4;
    } else if (buf[6] < 10) {
        result = 5;
    } else {
        err(3);
        return -3;
    }

    /* Size check */
    int size = (buf[9] << 8) | buf[10];
    if (size < 16) { err(4); return -4; }
    if (size > 1024) { warn(7); }
    if (size > 4096) { err(5); return -5; }

    /* Alignment check */
    if (size & 3) warn(8);
    if (size & 7) warn(9);

    /* Checksum region */
    if (buf[11] != (buf[0] ^ buf[1] ^ buf[2] ^ buf[3])) {
        warn(10);
    }
    if (buf[12] != (buf[4] ^ buf[5] ^ buf[6] ^ buf[7])) {
        warn(11);
    }

    /* Extended validation based on type */
    if (result == 1 && buf[13] > 50) {
        set_flag(9);
        if (buf[14] == 0xFF) set_flag(10);
        if (buf[14] == 0x00) set_flag(11);
    }
    if (result == 2 && buf[13] < 10) {
        set_flag(12);
        if (buf[14] > buf[15]) set_flag(13);
    }
    if (result == 3) {
        set_flag(14);
        if (buf[13] == buf[14] && buf[14] == buf[15]) set_flag(15);
    }

    /* Compatibility checks */
    if (buf[4] >= 3 && (buf[5] & 0x80)) {
        if (buf[6] > 1) { err(6); return -6; }
    }
    if (buf[4] <= 2 && (buf[5] & 0x40)) {
        warn(12);
    }
    if (buf[4] == 5 && buf[6] == 0) {
        if (buf[7] == 0) { err(7); return -7; }
    }

    return result;
}

int main() {
    unsigned char buf[16];
    klee_make_symbolic(buf, sizeof(buf), "buf");
    return validate_config(buf, 16);
}
