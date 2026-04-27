/*
 * Structural Unit 8: Multi-Level Dispatch
 * Source pattern: readelf (26 parser_dispatch functions, 516 switches total),
 *                 bison/prepare + prepare_symbol_definitions (10 dispatch),
 *                 tiffinfo (10 dispatch), nasm (6 dispatch)
 * CFG shape: Top-level dispatch (switch on section type) → each arm calls
 *            sub-handler which has its own dispatch. Two-level wide tree.
 *            Real-world: readelf dispatches on section type → display handler
 *            → sub-dispatch on record type.
 * Rule prediction: R4+R6 → random-path (multilevel dispatch, random-path
 *                  distributes across both levels, avoids starvation)
 */
#include <klee/klee.h>

int result;

/* Sub-handlers model readelf's display_debug_* → record type dispatch */
int handle_debug_info(unsigned char *data, int len) {
    int r = 0;
    for (int i = 0; i < len; i++) {
        switch (data[i] & 0x0F) {
            case 0: r += 1; break;
            case 1: r += data[i] >> 4; break;
            case 2: r -= 1; break;
            case 3: if (i + 1 < len) r += data[i+1]; break;
            case 4: r ^= 0xAA; break;
            case 5: r = (r << 1) | 1; break;
            case 6: r &= 0xFF; break;
            case 7: r |= (data[i] & 0xF0); break;
            default: r += i; break;
        }
    }
    return r;
}

int handle_debug_line(unsigned char *data, int len) {
    int r = 0;
    int line = 1, col = 0;
    for (int i = 0; i < len; i++) {
        unsigned char op = data[i];
        if (op == 0) { /* extended opcode */
            if (i + 1 < len) {
                switch (data[i+1] & 7) {
                    case 0: line = 1; col = 0; break;
                    case 1: r |= 0x100; break;
                    case 2: line += 100; break;
                    default: break;
                }
                i++;
            }
        } else if (op < 10) {
            /* Standard opcodes */
            switch (op) {
                case 1: line++; break;
                case 2: line--; break;
                case 3: col++; break;
                case 4: col--; break;
                case 5: r += line; break;
                case 6: r += col; break;
                case 7: line += (data[i] >> 4) + 1; break;
                case 8: col += (data[i] >> 4) + 1; break;
                case 9: r += line * col; break;
            }
        } else {
            /* Special opcode */
            line += (op - 10) / 5 + 1;
            col += (op - 10) % 5;
        }
    }
    return r + line * 1000 + col;
}

int handle_debug_frame(unsigned char *data, int len) {
    int r = 0;
    int reg[4] = {0, 0, 0, 0};
    for (int i = 0; i < len; i++) {
        unsigned char hi = data[i] >> 6;
        unsigned char lo = data[i] & 0x3F;
        switch (hi) {
            case 0: /* advance_loc */
                r += lo;
                break;
            case 1: /* offset */
                if (lo < 4) reg[lo] = (i + 1 < len) ? data[i+1] : 0;
                break;
            case 2: /* restore */
                if (lo < 4) r += reg[lo];
                break;
            case 3: /* extended */
                switch (lo & 7) {
                    case 0: r = 0; break;
                    case 1: reg[0] = reg[1]; break;
                    case 2: reg[2] = reg[3]; break;
                    case 3: r += reg[0] + reg[1]; break;
                    case 4: r += reg[2] + reg[3]; break;
                    case 5: r ^= reg[0]; break;
                    case 6: r ^= reg[2]; break;
                    case 7: r = reg[0] * 2 + reg[2]; break;
                }
                break;
        }
    }
    return r;
}

int handle_debug_abbrev(unsigned char *data, int len) {
    int count = 0;
    int tag_sum = 0;
    for (int i = 0; i < len; i++) {
        if (data[i] == 0) { count++; continue; }
        tag_sum += data[i];
        if (data[i] > 200) tag_sum -= 100;
        if (data[i] < 50)  tag_sum += 50;
    }
    return count * 100 + (tag_sum & 0xFF);
}

/* Top-level dispatch: models readelf's process_section_headers */
int process_section(unsigned char section_type, unsigned char *data, int len) {
    switch (section_type) {
        case 0: return 0; /* NULL */
        case 1: return handle_debug_info(data, len);
        case 2: return handle_debug_line(data, len);
        case 3: return handle_debug_frame(data, len);
        case 4: return handle_debug_abbrev(data, len);
        case 5: return handle_debug_info(data, len) + handle_debug_abbrev(data, len);
        case 6: return handle_debug_line(data, len) + handle_debug_frame(data, len);
        case 7: return handle_debug_info(data, len) + handle_debug_line(data, len);
        default: return -1;
    }
}

int main() {
    unsigned char input[10];
    klee_make_symbolic(input, sizeof(input), "input");

    /* Process two sections from the input */
    int r1 = process_section(input[0] & 7, input + 1, 4);
    int r2 = process_section(input[5] & 7, input + 6, 4);

    return r1 + r2;
}
