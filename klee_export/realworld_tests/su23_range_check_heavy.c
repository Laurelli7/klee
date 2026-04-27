/*
 * Structural Unit 23: Range-Check Heavy
 * Source: range_check_heavy (208 functions across ALL 8 programs)
 *         readelf bounds validation, bison array bounds, nasm operand ranges,
 *         tiffinfo tag value validation.
 * CFG: Many comparison+branch pairs checking if values are within ranges.
 *      Forms a sequence of (compare, branch) pairs with early-return on failure.
 *      Different from equality_chain (SU17) which checks for exact matches.
 *      Different from diamond_heavy (SU14) which reconverges; here early returns
 *      prune exploration.
 * Hypothesis: Each range check eliminates some paths early. The structure
 *   is deep and narrow. Passing ALL range checks requires a specific input.
 * Prediction: nurs:md2u (closest-to-uncovered helps reach deeper checks)
 */
#include <klee/klee.h>

struct packet {
    unsigned char header;
    unsigned char version;
    unsigned short length;
    unsigned char type;
    unsigned char subtype;
    unsigned short checksum;
    unsigned char payload[4];
};

int validate_packet(struct packet *pkt) {
    /* Layer 1: Header magic */
    if (pkt->header < 0xA0 || pkt->header > 0xAF)
        return -1;

    /* Layer 2: Version range */
    if (pkt->version < 1 || pkt->version > 5)
        return -2;

    /* Layer 3: Length bounds */
    if (pkt->length < 8 || pkt->length > 1024)
        return -3;

    /* Layer 4: Type range */
    if (pkt->type > 15)
        return -4;

    /* Layer 5: Subtype depends on type */
    if (pkt->type < 4) {
        if (pkt->subtype > 3) return -5;
    } else if (pkt->type < 8) {
        if (pkt->subtype < 4 || pkt->subtype > 7) return -6;
    } else if (pkt->type < 12) {
        if (pkt->subtype < 8 || pkt->subtype > 11) return -7;
    } else {
        if (pkt->subtype < 12) return -8;
    }

    /* Layer 6: Checksum range */
    unsigned short expected_low = (unsigned short)(pkt->type * 100 + pkt->subtype * 10);
    unsigned short expected_high = expected_low + 50;
    if (pkt->checksum < expected_low || pkt->checksum > expected_high)
        return -9;

    /* Layer 7: Payload byte ranges */
    if (pkt->payload[0] < 0x20 || pkt->payload[0] > 0x7E)
        return -10;
    if (pkt->payload[1] < pkt->payload[0])
        return -11;
    if (pkt->payload[2] > pkt->payload[1])
        return -12;
    if (pkt->payload[3] < 0x30 || pkt->payload[3] > 0x39)
        return -13;

    /* Layer 8: Cross-field range checks */
    int combo = pkt->version * pkt->type;
    if (combo < 5 || combo > 60)
        return -14;

    /* Layer 9: Version-specific length constraints */
    if (pkt->version <= 2 && pkt->length > 256)
        return -15;
    if (pkt->version >= 4 && pkt->length < 64)
        return -16;

    /* All checks passed — return payload sum */
    int sum = 0;
    for (int i = 0; i < 4; i++)
        sum += pkt->payload[i];
    return sum;
}

int main() {
    struct packet pkt;
    klee_make_symbolic(&pkt, sizeof(pkt), "packet");
    return validate_packet(&pkt);
}
