/*
 * Structural Unit 19: Struct-Field-Heavy Processing
 * Source: struct_field_heavy (143 functions across 7 programs)
 *         flvmeta AMF handlers (46 functions!), readelf ELF headers,
 *         bison symbol definitions, tiffinfo tag processing
 * CFG: Sequential access to struct fields via GEP. Each field access
 *      branches on the field's value. Heavy use of getelementptr.
 *      Different from SU15 (serialization) — here the fields are accessed
 *      via struct member offsets, not sequential byte parsing.
 * Hypothesis: Branching on struct fields is inherently independent
 *   (each field is a different byte). Graph is wide + shallow.
 * Prediction: nurs:covnew (R4: independent field-based branches)
 */
#include <klee/klee.h>

struct config {
    unsigned char version;
    unsigned char mode;
    unsigned char flags;
    unsigned char priority;
    unsigned short width;
    unsigned short height;
    unsigned char channels;
    unsigned char depth;
    unsigned char compression;
    unsigned char quality;
};

int validate_config(struct config *cfg) {
    int score = 0;

    /* Version check */
    if (cfg->version == 1) score += 10;
    else if (cfg->version == 2) score += 20;
    else if (cfg->version == 3) score += 30;
    else return -1;

    /* Mode-dependent validation */
    if (cfg->mode == 0) {
        /* mode 0: basic — only check version */
        score += 5;
    } else if (cfg->mode == 1) {
        /* mode 1: extended — check dimensions */
        if (cfg->width > 0 && cfg->height > 0)
            score += cfg->version * 10;
        else
            return -2;
    } else if (cfg->mode == 2) {
        /* mode 2: full — check everything */
        if (cfg->width == 0 || cfg->height == 0) return -3;
        if (cfg->channels == 0 || cfg->channels > 4) return -4;
        if (cfg->depth != 8 && cfg->depth != 16 && cfg->depth != 32) return -5;
        score += 100;
    } else {
        return -6;
    }

    /* Flag processing */
    if (cfg->flags & 0x01) {
        score += 1;
        if (cfg->compression > 5) return -7;
    }
    if (cfg->flags & 0x02) {
        score += 2;
        if (cfg->quality == 0) return -8;
    }
    if (cfg->flags & 0x04) {
        score += 4;
    }
    if (cfg->flags & 0x08) {
        score += 8;
        if (cfg->mode != 2) return -9;
    }
    if (cfg->flags & 0x10) {
        score += 16;
        if (cfg->priority > 10) return -10;
    }

    /* Cross-field validation */
    if (cfg->mode == 2 && cfg->version >= 2) {
        int pixels = cfg->width * cfg->height;
        if (pixels > 60000) {
            if (cfg->compression == 0) return -11;
        }
        if (cfg->channels * cfg->depth > 64) return -12;
    }

    /* Priority affects score */
    score += cfg->priority;
    if (cfg->priority > 5 && cfg->quality < 50)
        score -= 20;

    return score;
}

int main() {
    struct config cfg;
    klee_make_symbolic(&cfg, sizeof(cfg), "config");
    return validate_config(&cfg);
}
