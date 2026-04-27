// exp55: "Symbolic Memcpy" — Both offset and length of a copy operation
// are symbolic, creating O(offset × length) state space from memory
// resolution. This is a common real-world pattern in parsers.
//
// Structure: A 32-byte source buffer, symbolic offset (0-24) and
// length (1-8). For each byte copied, KLEE must resolve the symbolic
// source address and potentially fork. Longer copies = more forks.
//
// Key insight: NURS:qc should prefer short copies (cheaper solver),
// while md2u should prefer copies that reach the post-copy code.
// covnew should be indifferent (all copies cover same code).
//
// Expected: qc finds short-copy paths fast. DFS picks one path and
// follows it. The critical question is whether long-copy paths
// create enough solver overhead to differentiate qc from others.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int process_short(uint8_t *buf) {
    return buf[0] + buf[1];
}
__attribute__((noinline)) int process_medium(uint8_t *buf, int len) {
    int sum = 0;
    for (int i = 0; i < len && i < 4; i++) sum += buf[i];
    return sum;
}
__attribute__((noinline)) int process_long(uint8_t *buf, int len) {
    int xor = 0;
    for (int i = 0; i < len && i < 8; i++) xor ^= buf[i];
    return xor;
}

int main() {
    uint8_t source[32];
    uint8_t dest[8];
    uint8_t offset, length;
    klee_make_symbolic(source, sizeof(source), "source");
    klee_make_symbolic(&offset, sizeof(offset), "offset");
    klee_make_symbolic(&length, sizeof(length), "length");

    // Constrain offset and length
    if (offset >= 24) return 0;
    if (length == 0 || length > 8) return 0;
    if (offset + length > 32) return 0;

    // Manual memcpy with symbolic offset and length
    // Each iteration can fork on the symbolic source address
    for (uint8_t i = 0; i < length; i++) {
        dest[i] = source[offset + i];
    }

    // Classification based on length
    if (length <= 2)     return process_short(dest);
    else if (length <= 4) return process_medium(dest, length);
    else                  return process_long(dest, length);
}
