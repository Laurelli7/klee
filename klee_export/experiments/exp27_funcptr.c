// exp27: "Function pointer dispatch" — A program that uses a symbolic
// index to dispatch through an array of function pointers.
// Tests how searchers handle indirect branches.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handler_0(int x) { return x + 10; }
__attribute__((noinline)) int handler_1(int x) { return x + 20; }
__attribute__((noinline)) int handler_2(int x) { return x + 30; }
__attribute__((noinline)) int handler_3(int x) { return x + 40; }
__attribute__((noinline)) int handler_4(int x) { return x + 50; }
__attribute__((noinline)) int handler_5(int x) { return x + 60; }
__attribute__((noinline)) int handler_6(int x) { return x + 70; }
__attribute__((noinline)) int handler_7(int x) { return x + 80; }

typedef int (*handler_fn)(int);

int main() {
    uint8_t cmd;
    uint8_t arg;
    klee_make_symbolic(&cmd, sizeof(cmd), "cmd");
    klee_make_symbolic(&arg, sizeof(arg), "arg");

    handler_fn handlers[] = {
        handler_0, handler_1, handler_2, handler_3,
        handler_4, handler_5, handler_6, handler_7
    };

    if (cmd < 8) {
        int result = handlers[cmd](arg);
        if (result == 42) return -1; // BUG
        return result;
    }
    return 0;
}
