/*
 * Structural Unit 10: Dense Switch Jump Table
 * Source: bison/gram_parse (128-151 cases, dense), lua VM dispatch (28 cases, dense)
 *         nasm instruction encoding (180 cases, dense)
 * CFG: Single large dense switch (consecutive case values 0..N) where
 *       each case has a unique handler with distinct code.
 *       Different from SU01 (loop around switch) and SU09 (grouped cases).
 *       Here: flat, one-shot, every case covers distinct instructions.
 * Hypothesis: Pure width, no loop → coverage heuristics directly effective
 * Prediction: nurs:covnew (R4 pure: many independent regions, each with novel code)
 */
#include <klee/klee.h>

int regs[4];
int mem[8];
int flags;

/* 32 distinct handlers — models a parser action table */
int execute_action(unsigned char action, unsigned char arg) {
    switch (action & 0x1F) {
        case 0:  regs[0] = arg; flags = 0; return 1;
        case 1:  regs[1] = arg; flags = 1; return 1;
        case 2:  regs[2] = arg; flags = 2; return 1;
        case 3:  regs[3] = arg; flags = 3; return 1;
        case 4:  regs[0] += regs[1]; flags |= 0x10; return 2;
        case 5:  regs[0] -= regs[1]; flags |= 0x20; return 2;
        case 6:  regs[2] += regs[3]; flags |= 0x40; return 2;
        case 7:  regs[2] -= regs[3]; flags |= 0x80; return 2;
        case 8:  mem[arg & 7] = regs[0]; return 3;
        case 9:  mem[arg & 7] = regs[1]; return 3;
        case 10: mem[arg & 7] = regs[2]; return 3;
        case 11: mem[arg & 7] = regs[3]; return 3;
        case 12: regs[0] = mem[arg & 7]; return 4;
        case 13: regs[1] = mem[arg & 7]; return 4;
        case 14: regs[2] = mem[arg & 7]; return 4;
        case 15: regs[3] = mem[arg & 7]; return 4;
        case 16: if (flags & 1) regs[0] = regs[1]; return 5;
        case 17: if (flags & 2) regs[0] = regs[2]; return 5;
        case 18: if (flags & 4) regs[0] = regs[3]; return 5;
        case 19: if (flags & 8) regs[1] = regs[3]; return 5;
        case 20: regs[0] = regs[0] & arg; flags = (regs[0] == 0); return 6;
        case 21: regs[0] = regs[0] | arg; flags = (regs[0] == 0); return 6;
        case 22: regs[0] = regs[0] ^ arg; flags = (regs[0] == 0); return 6;
        case 23: regs[0] = ~regs[0]; flags = (regs[0] == 0); return 6;
        case 24: { int t = regs[0]; regs[0] = regs[1]; regs[1] = t; return 7; }
        case 25: { int t = regs[2]; regs[2] = regs[3]; regs[3] = t; return 7; }
        case 26: flags = (regs[0] > regs[1]) ? 1 : (regs[0] < regs[1]) ? -1 : 0; return 8;
        case 27: flags = (regs[2] > regs[3]) ? 1 : (regs[2] < regs[3]) ? -1 : 0; return 8;
        case 28: regs[0] = regs[1] + regs[2] + regs[3]; return 9;
        case 29: regs[0] = (regs[1] << (arg & 3)) | (regs[2] >> (4 - (arg & 3))); return 9;
        case 30: if (regs[0] == arg) flags = 0x100; else flags = 0; return 10;
        case 31: if (regs[0] != arg) flags = 0x200; else flags = 0; return 10;
    }
    return 0;
}

int main() {
    unsigned char program[8];
    klee_make_symbolic(program, sizeof(program), "program");

    int result = 0;
    /* Only 4 dispatches — no loop domination */
    result += execute_action(program[0], program[1]);
    result += execute_action(program[2], program[3]);
    result += execute_action(program[4], program[5]);
    result += execute_action(program[6], program[7]);
    return result;
}
