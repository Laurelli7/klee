/*
 * Structural Unit 1: Large Switch Dispatch
 * Source pattern: bc/execute (91 cases), readelf/decode_location_expression (427),
 *                 lua VM loop (28 cases), nasm instruction encoding (180 cases)
 * CFG shape: Single symbolic input dispatches to many independent arms,
 *            each arm does trivial work. Wide, shallow tree.
 * Rule prediction: R4 → random-path (wide switch, independent arms)
 */
#include <klee/klee.h>

int accumulator;
int regs[8];

void op_nop(void) { /* nothing */ }
void op_add(int a, int b) { regs[a & 7] += regs[b & 7]; }
void op_sub(int a, int b) { regs[a & 7] -= regs[b & 7]; }
void op_mul(int a, int b) { regs[a & 7] *= regs[b & 7]; }
void op_load(int r, int v) { regs[r & 7] = v; }
void op_store(int r) { accumulator = regs[r & 7]; }
void op_inc(int r) { regs[r & 7]++; }
void op_dec(int r) { regs[r & 7]--; }

int execute_opcode(unsigned char opcode, int arg1, int arg2) {
    switch (opcode) {
        case 0x00: op_nop(); return 0;
        case 0x01: op_add(arg1, arg2); return 1;
        case 0x02: op_sub(arg1, arg2); return 1;
        case 0x03: op_mul(arg1, arg2); return 1;
        case 0x04: op_load(arg1, arg2); return 1;
        case 0x05: op_store(arg1); return 1;
        case 0x06: op_inc(arg1); return 1;
        case 0x07: op_dec(arg1); return 1;
        case 0x08: regs[0] = regs[1] + regs[2]; return 2;
        case 0x09: regs[0] = regs[1] - regs[2]; return 2;
        case 0x0A: regs[0] = regs[1] & regs[2]; return 2;
        case 0x0B: regs[0] = regs[1] | regs[2]; return 2;
        case 0x0C: regs[0] = regs[1] ^ regs[2]; return 2;
        case 0x0D: regs[0] = ~regs[1]; return 2;
        case 0x0E: regs[0] = regs[1] << (arg1 & 7); return 2;
        case 0x0F: regs[0] = regs[1] >> (arg1 & 7); return 2;
        case 0x10: accumulator = regs[0] + regs[1]; return 3;
        case 0x11: accumulator = regs[0] - regs[1]; return 3;
        case 0x12: accumulator = regs[0] * regs[1]; return 3;
        case 0x13: accumulator = regs[0] & regs[1]; return 3;
        case 0x14: accumulator = regs[0] | regs[1]; return 3;
        case 0x15: accumulator = regs[0] ^ regs[1]; return 3;
        case 0x16: accumulator = regs[2] + regs[3]; return 3;
        case 0x17: accumulator = regs[2] - regs[3]; return 3;
        case 0x18: accumulator = regs[2] * regs[3]; return 3;
        case 0x19: accumulator = regs[4] + regs[5]; return 3;
        case 0x1A: accumulator = regs[4] - regs[5]; return 3;
        case 0x1B: accumulator = regs[4] * regs[5]; return 3;
        case 0x1C: accumulator = regs[6] + regs[7]; return 3;
        case 0x1D: accumulator = regs[6] - regs[7]; return 3;
        case 0x1E: accumulator = regs[6] * regs[7]; return 3;
        case 0x1F: op_nop(); op_nop(); return 4;
        case 0x20: regs[arg1 & 7] = accumulator; return 4;
        case 0x21: regs[arg1 & 7] = accumulator + 1; return 4;
        case 0x22: regs[arg1 & 7] = accumulator - 1; return 4;
        case 0x23: regs[arg1 & 7] = -accumulator; return 4;
        case 0x24: if (accumulator > 0) regs[0] = 1; return 5;
        case 0x25: if (accumulator < 0) regs[0] = -1; return 5;
        case 0x26: if (accumulator == 0) regs[0] = 0; return 5;
        case 0x27: if (accumulator != 0) regs[1] = 1; return 5;
        case 0x28: regs[0] = arg1 + arg2; return 6;
        case 0x29: regs[1] = arg1 - arg2; return 6;
        case 0x2A: regs[2] = arg1 * arg2; return 6;
        case 0x2B: regs[3] = arg1 & arg2; return 6;
        case 0x2C: regs[4] = arg1 | arg2; return 6;
        case 0x2D: regs[5] = arg1 ^ arg2; return 6;
        case 0x2E: regs[6] = arg1 + 1; return 6;
        case 0x2F: regs[7] = arg2 - 1; return 6;
        default: return -1;
    }
}

int main() {
    unsigned char program[6];
    klee_make_symbolic(program, sizeof(program), "program");

    int total = 0;
    for (int i = 0; i < 6; i++) {
        total += execute_opcode(program[i], program[(i+1) % 6], program[(i+2) % 6]);
    }
    return total;
}
