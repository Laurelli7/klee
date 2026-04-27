// exp57: "Pointer Chase" — A linked-structure traversal where
// the traversal order depends on symbolic data. Each node has a
// symbolic "next" index, creating a pointer-chase pattern.
//
// Structure: 8 nodes in an array. Each node has a value and a
// symbolic next index. Following the chain for N steps creates
// N forks of 8 possibilities each = 8^N states.
//
// This pattern appears in real programs (hash table traversal,
// linked list walks, graph traversal with symbolic edges).
//
// DFS: follows one chain completely, then backtracks
// BFS: tries all 8 choices at each step, 8→64→512→4096 states
// covnew: each node has unique code, should diversify visits
// random-path: tree sampling, should visit diverse chains
//
// Expected: DFS fast for short chains. BFS explodes for long chains.
// covnew should visit all 8 unique node handlers efficiently.
// random-path should sample diverse chains.
#include "klee/klee.h"
#include <stdint.h>

typedef struct {
    int value;
    int (*handler)(int);
} Node;

__attribute__((noinline)) int handle_0(int x) { return x + 100; }
__attribute__((noinline)) int handle_1(int x) { return x + 200; }
__attribute__((noinline)) int handle_2(int x) { return x + 300; }
__attribute__((noinline)) int handle_3(int x) { return x + 400; }
__attribute__((noinline)) int handle_4(int x) { return x + 500; }
__attribute__((noinline)) int handle_5(int x) { return x + 600; }
__attribute__((noinline)) int handle_6(int x) { return x + 700; }
__attribute__((noinline)) int handle_7(int x) { return x + 800; }

int main() {
    uint8_t sequence[5]; // 5 steps through the node array
    klee_make_symbolic(sequence, sizeof(sequence), "seq");

    Node nodes[8];
    nodes[0].value = 10; nodes[0].handler = handle_0;
    nodes[1].value = 20; nodes[1].handler = handle_1;
    nodes[2].value = 30; nodes[2].handler = handle_2;
    nodes[3].value = 40; nodes[3].handler = handle_3;
    nodes[4].value = 50; nodes[4].handler = handle_4;
    nodes[5].value = 60; nodes[5].handler = handle_5;
    nodes[6].value = 70; nodes[6].handler = handle_6;
    nodes[7].value = 80; nodes[7].handler = handle_7;

    int result = 0;

    // Chase: 5 steps, each step picks a node by symbolic index
    for (int step = 0; step < 5; step++) {
        uint8_t idx = sequence[step] & 0x07; // 8 possible nodes
        result = nodes[idx].handler(result);
        result += nodes[idx].value;

        // Self-loop detection: if we'd visit the same node twice in a row,
        // take a penalty path instead
        if (step > 0 && (sequence[step] & 0x07) == (sequence[step-1] & 0x07)) {
            result -= 50; // penalty for revisiting
        }
    }

    // Final classification based on which nodes were visited
    if (result > 3000) return 1;  // visited high-value nodes
    if (result < 1000) return 2;  // visited low-value nodes
    return 0;                     // mixed
}
