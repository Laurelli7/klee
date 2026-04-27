/*
 * Structural Unit 11: Sparse Switch (Hash-Like Dispatch)
 * Source: nasm/load_code sparse_switch (non-consecutive case values),
 *         readelf relocation dispatch (427 cases, sparse enum values)
 * CFG: Switch where case values are sparse (e.g., 0, 5, 17, 42, 100, ...),
 *      so LLVM generates cascaded if-else comparison tree instead of jump table.
 *      Creates deep binary search tree instead of flat dispatch.
 * Hypothesis: Sparse switch → binary search tree in IR → more like deep chain
 *   than wide dispatch. DFS may reach specific cases faster.
 * Prediction: DFS (R1-like: binary search tree creates sequential gating to each case)
 */
#include <klee/klee.h>

int handle_sparse_opcode(unsigned short opcode) {
    /* Models hash/enum values that aren't consecutive */
    switch (opcode) {
        case 0x0001: return 10;
        case 0x0003: return 11;
        case 0x0007: return 12;
        case 0x000F: return 13;
        case 0x001F: return 14;
        case 0x003F: return 15;
        case 0x007F: return 16;
        case 0x00FF: return 17;
        case 0x0100: return 20;
        case 0x0200: return 21;
        case 0x0400: return 22;
        case 0x0800: return 23;
        case 0x1000: return 24;
        case 0x2000: return 25;
        case 0x4000: return 26;
        case 0x8000: return 27;
        case 0x0101: return 30;
        case 0x0202: return 31;
        case 0x0303: return 32;
        case 0x0404: return 33;
        case 0x1234: return 40;
        case 0x5678: return 41;
        case 0x9ABC: return 42;
        case 0xDEF0: return 43;
        case 0xAAAA: return 50;
        case 0x5555: return 51;
        case 0xF00D: return 52;
        case 0xBEEF: return 53;
        case 0xCAFE: return 54;
        case 0xFACE: return 55;
        default: return 0;
    }
}

int main() {
    unsigned char input[10];
    klee_make_symbolic(input, sizeof(input), "input");

    int total = 0;
    /* 5 independent sparse dispatches */
    for (int i = 0; i < 5; i++) {
        unsigned short opcode = ((unsigned short)input[i * 2] << 8) | input[i * 2 + 1];
        total += handle_sparse_opcode(opcode);
    }
    return total;
}
