// exp29: "Switch maze" — Large switch with 64 cases, some leading to
// deep paths, others to shallow exits. The switch itself should create
// a very wide fork at once. BFS should handle switches well (level-fair
// across cases), DFS will pick the first case and go deep.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int deep_handler(uint8_t x) {
    int r = x;
    if (x & 0x01) r += 10;
    if (x & 0x02) r += 20;
    if (x & 0x04) r += 30;
    if (x & 0x08) r += 40;
    if (x & 0x10) r += 50;
    if (x & 0x20) r += 60;
    if (x & 0x40) r += 70;
    if (x & 0x80) r += 80;
    return r;
}

int main() {
    uint8_t opcode, operand;
    klee_make_symbolic(&opcode, sizeof(opcode), "opcode");
    klee_make_symbolic(&operand, sizeof(operand), "operand");

    // Switch with 64 cases: first 4 are "deep", rest are "shallow"
    switch (opcode & 0x3F) { // 64 possible values
        case 0: return deep_handler(operand);
        case 1: return deep_handler(operand) + 1;
        case 2: return deep_handler(operand) + 2;
        case 3: return deep_handler(operand) + 3;
        case 4:  return 100;
        case 5:  return 101;
        case 6:  return 102;
        case 7:  return 103;
        case 8:  return 104;
        case 9:  return 105;
        case 10: return 106;
        case 11: return 107;
        case 12: return 108;
        case 13: return 109;
        case 14: return 110;
        case 15: return 111;
        case 16: return 112;
        case 17: return 113;
        case 18: return 114;
        case 19: return 115;
        case 20: return 116;
        case 21: return 117;
        case 22: return 118;
        case 23: return 119;
        case 24: return 120;
        case 25: return 121;
        case 26: return 122;
        case 27: return 123;
        case 28: return 124;
        case 29: return 125;
        case 30: return 126;
        case 31: return 127;
        default:
            if (operand == 0xFF) return -1; // BUG in default case
            return 200;
    }
}
