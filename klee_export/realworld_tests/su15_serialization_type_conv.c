/*
 * Structural Unit 15: Serialization-Heavy Type Conversion
 * Source: serialization_heavy (488 functions across all 8 programs!)
 *         tiffinfo/TIFFWriteDirectorySec (truncs + zexts + sexts),
 *         readelf ELF header parsing, bison parse table generation
 *         nasm binary output formatting
 * CFG: Sequential field processing: load → type-convert → validate → store.
 *      Each field has different width (8/16/32-bit) requiring different casts.
 *      Many zext/sext/trunc instructions. Branching on field type.
 * Hypothesis: Sequential processing with type variety creates many distinct
 *   code paths (different cast sequences). Coverage heuristics can distinguish.
 * Prediction: nurs:covnew (R4-like: each field type covers different instructions)
 */
#include <klee/klee.h>

struct record {
    unsigned char type;
    unsigned char field_count;
    unsigned char data[10];
};

int decode_field_u8(unsigned char *p) { return *p; }
int decode_field_u16(unsigned char *p) { return (p[0] << 8) | p[1]; }
int decode_field_s16(unsigned char *p) {
    int v = (p[0] << 8) | p[1];
    if (v > 32767) v -= 65536;
    return v;
}
int decode_field_u32(unsigned char *p) {
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}
int decode_field_packed(unsigned char *p) {
    /* Packed bitfields: 3 bits + 5 bits + 4 bits + 4 bits */
    int a = (p[0] >> 5) & 7;
    int b = p[0] & 0x1F;
    int c = (p[1] >> 4) & 0xF;
    int d = p[1] & 0xF;
    return a * 1000 + b * 100 + c * 10 + d;
}

int process_record(struct record *rec) {
    int result = 0;
    unsigned char *p = rec->data;
    int remaining = 10;
    int field_idx = 0;

    while (field_idx < rec->field_count && field_idx < 5 && remaining > 0) {
        unsigned char field_type = rec->type + field_idx;
        int value = 0;

        switch (field_type & 0x07) {
            case 0: /* u8 field */
                if (remaining < 1) return -1;
                value = decode_field_u8(p);
                p++; remaining--;
                break;
            case 1: /* u16 field */
                if (remaining < 2) return -2;
                value = decode_field_u16(p);
                p += 2; remaining -= 2;
                break;
            case 2: /* s16 field */
                if (remaining < 2) return -3;
                value = decode_field_s16(p);
                p += 2; remaining -= 2;
                break;
            case 3: /* u32 field */
                if (remaining < 4) return -4;
                value = decode_field_u32(p);
                p += 4; remaining -= 4;
                break;
            case 4: /* packed bitfield */
                if (remaining < 2) return -5;
                value = decode_field_packed(p);
                p += 2; remaining -= 2;
                break;
            case 5: /* repeated u8 */
                if (remaining < 3) return -6;
                value = p[0] + p[1] + p[2];
                p += 3; remaining -= 3;
                break;
            case 6: /* string length + data */
                {
                    if (remaining < 1) return -7;
                    int len = p[0];
                    if (len > remaining - 1) len = remaining - 1;
                    int hash = 0;
                    for (int j = 0; j < len && j < 4; j++)
                        hash = hash * 31 + p[1 + j];
                    value = hash;
                    p += 1 + len; remaining -= 1 + len;
                }
                break;
            case 7: /* flag byte with validation */
                if (remaining < 1) return -8;
                if (p[0] & 0x80) value = -(p[0] & 0x7F);
                else value = p[0];
                p++; remaining--;
                break;
        }

        /* Cross-field validation */
        if (field_idx > 0 && value < 0) {
            result |= (1 << field_idx);
        }
        result += value;
        field_idx++;
    }

    return result;
}

int main() {
    struct record rec;
    klee_make_symbolic(&rec, sizeof(rec), "record");
    return process_record(&rec);
}
