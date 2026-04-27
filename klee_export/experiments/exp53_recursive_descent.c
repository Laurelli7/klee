// exp53: "Recursive Descent Parser" — A recursive parser for a tiny
// grammar where symbolic input determines recursion depth.
//
// Grammar: expr = atom | '(' expr ')' | expr '+' expr
// Represented as: input bytes encode operations.
//   0x00-0x3F = atom (value = byte & 0x3F)
//   0x40-0x7F = '(' recurse ')'
//   0x80-0xBF = '+' (binary: parse left, parse right)
//   0xC0-0xFF = end
//
// This creates exponentially different path DEPTHS depending on
// whether input triggers recursion or stays flat. Stack depth varies
// from 1 to ~8.
//
// DFS: follows deepest recursion first (potentially very deep)
// BFS: tries all tokens at each depth 
// covnew: each recursion level has unique code (stack frames)
// md2u: distance to return varies with recursion depth
//
// Expected: Deep structural differences. DFS either very fast (lucky)
// or very slow (unlucky recursion). BFS moderate. Coverage-guided good.
#include "klee/klee.h"
#include <stdint.h>

static uint8_t *cursor;
static uint8_t *end;

__attribute__((noinline)) int eval_atom(int v) { return v; }
__attribute__((noinline)) int eval_add(int a, int b) { return a + b; }
__attribute__((noinline)) int eval_paren(int v) { return v; }
__attribute__((noinline)) int eval_error(void) { return -9999; }

int parse_expr(int depth) {
    if (depth > 6 || cursor >= end) return eval_error();
    
    uint8_t tok = *cursor;
    cursor++;

    if (tok < 0x40) {
        // Atom
        return eval_atom(tok & 0x3F);
    } else if (tok < 0x80) {
        // Parenthesized expression
        int inner = parse_expr(depth + 1);
        return eval_paren(inner);
    } else if (tok < 0xC0) {
        // Addition: parse two sub-expressions
        int left = parse_expr(depth + 1);
        int right = parse_expr(depth + 1);
        return eval_add(left, right);
    } else {
        // End token
        return eval_atom(0);
    }
}

int main() {
    uint8_t input[8];
    klee_make_symbolic(input, sizeof(input), "input");

    cursor = input;
    end = input + 8;

    int result = parse_expr(0);

    // Classification of result
    if (result < 0)    return -1;  // error path
    if (result < 10)   return 1;   // small
    if (result < 100)  return 2;   // medium
    if (result > 1000) return 3;   // overflow (many additions)
    return result;
}
