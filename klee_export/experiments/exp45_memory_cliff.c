// exp45: "Memory Cliff" — Tests how searchers behave under severe
// memory pressure. Allocates symbolic data on the heap, then branches
// extensively. DFS should complete paths and free memory; BFS should
// OOM. Run with --max-memory=100.
//
// Structure: Allocate 10 arrays of 1KB each with symbolic content,
// then branch on bits. DFS processes one path at a time (low memory).
// BFS holds ALL paths simultaneously (high memory).
//
// Expected: DFS finishes; BFS/random-path killed by OOM.
#include "klee/klee.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

__attribute__((noinline)) int process_A(int x) { return x + 111; }
__attribute__((noinline)) int process_B(int x) { return x + 222; }

int main() {
    uint8_t control[3]; // 3 bytes = 24 branches
    klee_make_symbolic(control, sizeof(control), "ctrl");

    // Create symbolic heap allocations that must stay alive
    char *bufs[5];
    for (int i = 0; i < 5; i++) {
        bufs[i] = (char*)malloc(512);
        if (!bufs[i]) return -1;
        klee_make_symbolic(bufs[i], 512, "buf");
    }

    int r = 0;

    // Branch on control bits (24 branches = 2^24 potential states)
    for (int i = 0; i < 3; i++) {
        if (control[i] & 0x01) r++;
        if (control[i] & 0x02) r++;
        if (control[i] & 0x04) r++;
        if (control[i] & 0x08) r++;
        if (control[i] & 0x10) r++;
        if (control[i] & 0x20) r++;
        if (control[i] & 0x40) r++;
        if (control[i] & 0x80) r++;
    }

    // Use buffer contents to prevent optimization
    if (bufs[0][0] == 'X') r = process_A(r);
    if (bufs[1][0] == 'Y') r = process_B(r);

    for (int i = 0; i < 5; i++) free(bufs[i]);
    return r;
}
