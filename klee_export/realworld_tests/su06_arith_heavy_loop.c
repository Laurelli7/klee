/*
 * Structural Unit 6: Arithmetic-Heavy Loop
 * Source pattern: bc/bc_divide (29 branches, 3 div/mod + 9 mul),
 *                 tiffinfo/TIFFComputeTile (9 div/mod + 9 mul),
 *                 bison/strversion_to_int (567 branches, 20 div/mod + 40 mul),
 *                 nasm/coff_section_header (13 div/mod)
 * CFG shape: Loop body with expensive arithmetic (division, modulo),
 *            conditions depend on arithmetic results. Solver-heavy.
 * Rule prediction: R11 → nurs:md2u (solver-expensive, md2u avoids hardest paths)
 */
#include <klee/klee.h>

/* Models bc's arbitrary-precision division and bison's version parsing */
int bignum_divide(unsigned char *num, int num_len, unsigned char divisor) {
    if (divisor == 0) return -1;

    int remainder = 0;
    int quotient_digits = 0;
    int nonzero_seen = 0;

    for (int i = 0; i < num_len; i++) {
        remainder = remainder * 256 + num[i];
        int q = remainder / divisor;
        remainder = remainder % divisor;
        if (q != 0) nonzero_seen = 1;
        if (nonzero_seen) quotient_digits++;
    }

    return quotient_digits * 1000 + remainder;
}

/* Models tiffinfo tile computation */
int compute_tile_offset(int width, int height, int tile_w, int tile_h, int tile_idx) {
    if (tile_w <= 0 || tile_h <= 0) return -1;

    int tiles_across = (width + tile_w - 1) / tile_w;
    int tiles_down = (height + tile_h - 1) / tile_h;
    int total_tiles = tiles_across * tiles_down;

    if (tile_idx < 0 || tile_idx >= total_tiles) return -2;

    int row = tile_idx / tiles_across;
    int col = tile_idx % tiles_across;

    int x_off = col * tile_w;
    int y_off = row * tile_h;

    /* Partial tile at edges */
    int actual_w = tile_w;
    int actual_h = tile_h;
    if (x_off + tile_w > width) actual_w = width - x_off;
    if (y_off + tile_h > height) actual_h = height - y_off;

    return actual_w * actual_h;
}

/* Models GCD-based rational approximation (from tiffinfo/ToRationalEuclideanGCD) */
int rational_approx(int num, int den) {
    if (den == 0) return -1;
    if (num < 0) num = -num;
    if (den < 0) den = -den;

    int a = num, b = den;
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    /* a is GCD */
    if (a == 0) return 0;
    return (num / a) * 100 + (den / a);
}

int main() {
    unsigned char num[6];
    unsigned char params[8];
    klee_make_symbolic(num, sizeof(num), "num");
    klee_make_symbolic(params, sizeof(params), "params");

    int r1 = bignum_divide(num, 6, params[0]);

    /* Use params as tile dimensions (constrained to reasonable range) */
    int width  = (params[1] & 0x7F) + 1;   /* 1-128 */
    int height = (params[2] & 0x7F) + 1;
    int tw = (params[3] & 0x1F) + 1;       /* 1-32 */
    int th = (params[4] & 0x1F) + 1;
    int tidx = params[5] & 0x3F;

    int r2 = compute_tile_offset(width, height, tw, th, tidx);

    int r3 = rational_approx((int)(params[6]) - 128, (int)(params[7]) - 128);

    return r1 + r2 + r3;
}
