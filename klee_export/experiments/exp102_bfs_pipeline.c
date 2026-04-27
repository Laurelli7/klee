// exp102: "BFS Pipeline" — A realistic pipeline pattern: input parsing →
// validation → processing → output. Each stage has independent branches.
// Real-world code often looks like this (web request handling, compilers).
//
// The key: stages are SEQUENTIAL (you must pass through parseformat
// before reaching validate), but within each stage, branches are
// INDEPENDENT. BFS explores all format branches before validate branches,
// etc., giving systematic coverage. DFS goes through one path end-to-end.
//
// Unlike exp99, here the stages have DIFFERENT numbers of branches and
// some branches in later stages are only reachable from certain earlier
// branches (data dependency via constrained values).
//
// Expected: BFS > DFS due to systematic breadth-first stage exploration
#include "klee/klee.h"
#include <stdint.h>

// Parse stage: 4 formats
__attribute__((noinline)) int parse_json(int x) { return x | 0x10; }
__attribute__((noinline)) int parse_xml(int x)  { return x | 0x20; }
__attribute__((noinline)) int parse_csv(int x)  { return x | 0x30; }
__attribute__((noinline)) int parse_bin(int x)  { return x | 0x40; }

// Validate stage: 3 levels
__attribute__((noinline)) int validate_strict(int x) { return x | 0x100; }
__attribute__((noinline)) int validate_loose(int x)  { return x | 0x200; }
__attribute__((noinline)) int validate_none(int x)   { return x | 0x300; }

// Process stage: 8 operations
__attribute__((noinline)) int proc_sum(int x)     { return x + 1000; }
__attribute__((noinline)) int proc_avg(int x)     { return x + 2000; }
__attribute__((noinline)) int proc_max(int x)     { return x + 3000; }
__attribute__((noinline)) int proc_min(int x)     { return x + 4000; }
__attribute__((noinline)) int proc_count(int x)   { return x + 5000; }
__attribute__((noinline)) int proc_median(int x)  { return x + 6000; }
__attribute__((noinline)) int proc_mode(int x)    { return x + 7000; }
__attribute__((noinline)) int proc_stddev(int x)  { return x + 8000; }

// Output stage: 4 formats
__attribute__((noinline)) int out_text(int x)  { return x ^ 0x1000; }
__attribute__((noinline)) int out_html(int x)  { return x ^ 0x2000; }
__attribute__((noinline)) int out_json(int x)  { return x ^ 0x3000; }
__attribute__((noinline)) int out_binary(int x) { return x ^ 0x4000; }

// Error handlers (unique coverage from error paths)
__attribute__((noinline)) int err_parse(int x) { return -(x + 1); }
__attribute__((noinline)) int err_validate(int x) { return -(x + 2); }
__attribute__((noinline)) int err_process(int x) { return -(x + 3); }

int main() {
    uint8_t format, validate, operation, output;
    uint8_t err_trigger;
    uint8_t padding[3]; // state explosion padding
    klee_make_symbolic(&format, sizeof(format), "fmt");
    klee_make_symbolic(&validate, sizeof(validate), "val");
    klee_make_symbolic(&operation, sizeof(operation), "op");
    klee_make_symbolic(&output, sizeof(output), "out");
    klee_make_symbolic(&err_trigger, sizeof(err_trigger), "err");
    klee_make_symbolic(padding, sizeof(padding), "pad");

    int r = 0;

    // Stage 1: Parse (4 branches)
    switch (format >> 6) {
        case 0: r = parse_json(r); break;
        case 1: r = parse_xml(r); break;
        case 2: r = parse_csv(r); break;
        case 3: r = parse_bin(r); break;
    }

    // Error check after parse
    if (err_trigger & 0x01) {
        r = err_parse(r);
        return r;  // Early exit — DFS might take this and miss later stages
    }

    // DFS trap: state explosion between stages
    if (padding[0] & 0x01) r++;
    if (padding[0] & 0x02) r++;
    if (padding[0] & 0x04) r++;
    if (padding[0] & 0x08) r++;
    if (padding[0] & 0x10) r++;
    if (padding[0] & 0x20) r++;
    if (padding[0] & 0x40) r++;
    if (padding[0] & 0x80) r++;

    // Stage 2: Validate (3 branches)
    uint8_t vlevel = validate % 3;
    if (vlevel == 0) r = validate_strict(r);
    else if (vlevel == 1) r = validate_loose(r);
    else r = validate_none(r);

    // Error check after validate
    if (err_trigger & 0x02) {
        r = err_validate(r);
        return r;
    }

    // More state explosion
    if (padding[1] & 0x01) r++;
    if (padding[1] & 0x02) r++;
    if (padding[1] & 0x04) r++;
    if (padding[1] & 0x08) r++;
    if (padding[1] & 0x10) r++;
    if (padding[1] & 0x20) r++;
    if (padding[1] & 0x40) r++;
    if (padding[1] & 0x80) r++;

    // Stage 3: Process (8 branches)
    switch (operation >> 5) {
        case 0: r = proc_sum(r); break;
        case 1: r = proc_avg(r); break;
        case 2: r = proc_max(r); break;
        case 3: r = proc_min(r); break;
        case 4: r = proc_count(r); break;
        case 5: r = proc_median(r); break;
        case 6: r = proc_mode(r); break;
        case 7: r = proc_stddev(r); break;
    }

    // Error check after process
    if (err_trigger & 0x04) {
        r = err_process(r);
        return r;
    }

    // More state explosion
    if (padding[2] & 0x01) r++;
    if (padding[2] & 0x02) r++;
    if (padding[2] & 0x04) r++;
    if (padding[2] & 0x08) r++;
    if (padding[2] & 0x10) r++;
    if (padding[2] & 0x20) r++;
    if (padding[2] & 0x40) r++;
    if (padding[2] & 0x80) r++;

    // Stage 4: Output (4 branches)
    switch (output >> 6) {
        case 0: r = out_text(r); break;
        case 1: r = out_html(r); break;
        case 2: r = out_json(r); break;
        case 3: r = out_binary(r); break;
    }

    return r;
}
