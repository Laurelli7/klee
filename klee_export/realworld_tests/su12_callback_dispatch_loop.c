/*
 * Structural Unit 12: Callback Dispatch + Plugin Loop
 * Source: flvmeta/flv_parse (callback_dispatch + plugin_loop),
 *         tiffinfo codec dispatch, readelf section handler callbacks
 *         bison plugin_loop (5 programs have this pattern)
 * CFG: Loop iterates over items; each item selects a handler via function pointer
 *      (indirect call). The handler itself may have complex branching.
 *      Key difference from SU08: dispatch is indirect (symbolic function pointer)
 *      rather than switch-based.
 * Hypothesis: Indirect calls create a fork per possible target; loop means
 *   these forks multiply per iteration → state explosion like R10
 *   but with added solver cost from function pointer resolution.
 * Prediction: DFS (R10: identical loop iterations + R9: coverage-blind
 *   after first iteration covers all handlers)
 */
#include <klee/klee.h>

typedef int (*handler_fn)(int data, int context);

static int handler_passthrough(int data, int context) {
    return data + context;
}

static int handler_accumulate(int data, int context) {
    return data * 2 + (context & 0xFF);
}

static int handler_check(int data, int context) {
    if (data > 100) return context + 50;
    if (data < -100) return context - 50;
    return context;
}

static int handler_bitwise(int data, int context) {
    return (data ^ context) & 0xFFFF;
}

static int handler_shift(int data, int context) {
    return ((data & 0xFF) << 4) | ((context >> 4) & 0x0F);
}

static int handler_negate(int data, int context) {
    return -(data + context);
}

static handler_fn handlers[] = {
    handler_passthrough,
    handler_accumulate,
    handler_check,
    handler_bitwise,
    handler_shift,
    handler_negate,
};

#define N_HANDLERS 6

int main() {
    unsigned char input[12];
    klee_make_symbolic(input, sizeof(input), "input");

    int context = 0;
    /* Loop with indirect dispatch */
    for (int i = 0; i < 6; i++) {
        int selector = input[i * 2] % N_HANDLERS;
        int data = (int)(signed char)input[i * 2 + 1];
        handler_fn fn = handlers[selector];
        context = fn(data, context);
    }
    return context;
}
