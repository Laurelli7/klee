/*
 * Structural Unit 24: Bitwise-Dominated (Non-Accumulator)
 * Source: bitwise_dominated (163 functions across ALL 8 programs)
 *         Different from SU13 (bitwise_accumulator_loop) which is about
 *         loop-carried bitwise accumulation. This pattern is about functions
 *         where bitwise ops dominate the computation but NOT in a loop.
 *         E.g., readelf flag decoding, nasm opcode encoding, lua tag manipulation.
 * CFG: Branching based on bit-tests (AND + compare), with each bit-test
 *      potentially independent. Creates a "bit-field tree" structure.
 * Hypothesis: Each bit is independently testable. Wide + shallow.
 *   Coverage-guided should efficiently enumerate bit combinations.
 * Prediction: nurs:covnew (independent bits = coverage-friendly)
 */
#include <klee/klee.h>

int decode_flags(unsigned short flags, unsigned char mode) {
    int result = 0;

    /* Primary flags (bits 0-7) */
    if (flags & 0x0001) result |= 0x01;   /* FLAG_READ */
    if (flags & 0x0002) result |= 0x02;   /* FLAG_WRITE */
    if (flags & 0x0004) result |= 0x04;   /* FLAG_EXEC */
    if (flags & 0x0008) {                  /* FLAG_SPECIAL */
        if (mode == 0) result |= 0x10;
        else if (mode == 1) result |= 0x20;
        else result |= 0x30;
    }
    if (flags & 0x0010) {                  /* FLAG_EXTENDED */
        /* Extended flag depends on primary flags */
        if ((result & 0x07) == 0x07)
            result |= 0x40;               /* all rwx → full access */
        else if (result & 0x04)
            result |= 0x80;               /* exec only → restricted */
    }

    /* Secondary flags (bits 8-11) */
    if (flags & 0x0100) {
        result += 100;
        if (flags & 0x0200)
            result += 200;                 /* combined secondary */
    }
    if (flags & 0x0400) {
        if (!(flags & 0x0100))
            result -= 50;                  /* orphan flag penalty */
        else
            result += 50;
    }
    if (flags & 0x0800) {
        result ^= 0xFF;                   /* invert lower bits */
    }

    /* Modifier flags (bits 12-15) */
    unsigned char mod = (flags >> 12) & 0x0F;
    switch (mod) {
        case 0: break;
        case 1: result <<= 1; break;
        case 2: result >>= 1; break;
        case 3: result = ~result; break;
        case 4: result &= 0xFF00; break;
        case 5: result |= 0x00FF; break;
        case 6: result ^= 0xAAAA; break;
        case 7: result = (result << 8) | (result >> 8); break;
        default:
            result += mod * 10;
            break;
    }

    /* Mode-dependent final adjustment */
    if (mode & 0x01) result += 1000;
    if (mode & 0x02) result -= 500;
    if (mode & 0x04) result *= 2;
    if (mode & 0x08) result = -result;
    if (mode & 0x10) result &= 0xFFFF;
    if (mode & 0x20) result |= 0x8000;
    if (mode & 0x40) result ^= 0x5555;
    if (mode & 0x80) result = (result >> 4) | (result << 4);

    return result;
}

int main() {
    unsigned short flags;
    unsigned char mode;
    klee_make_symbolic(&flags, sizeof(flags), "flags");
    klee_make_symbolic(&mode, sizeof(mode), "mode");
    return decode_flags(flags, mode);
}
