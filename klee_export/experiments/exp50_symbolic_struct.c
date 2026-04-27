// exp50: "Symbolic Struct" — Cross-field constraints in a struct create
// dependencies that challenge different searchers differently.
//
// Structure: A struct with 4 uint8 fields. Fields depend on each other:
// field B's valid range depends on field A's value, etc.
// This creates a "constraint funnel" where valid paths narrow as
// we add more constraints. Unique coverage behind each constraint.
//
// DFS: follows one constraint chain, may hit infeasible early
// BFS: tries all values of field A first, wasteful
// md2u: should see uncovered struct handlers at known distance
// covnew: should chase the unique handlers behind each constraint
//
// Expected: DFS does well (linear constraint chain matches DFS).
// BFS wastes time. covnew decent. md2u decent.
#include "klee/klee.h"
#include <stdint.h>

typedef struct {
    uint8_t tag;
    uint8_t len;
    uint8_t flags;
    uint8_t checksum;
} Packet;

__attribute__((noinline)) int parse_type1(Packet *p) { return p->len * 3 + 10; }
__attribute__((noinline)) int parse_type2(Packet *p) { return p->len * 7 + 20; }
__attribute__((noinline)) int parse_type3(Packet *p) { return p->len * 11 + 30; }
__attribute__((noinline)) int parse_type4(Packet *p) { return p->flags + 40; }
__attribute__((noinline)) int parse_corrupt(Packet *p) { return -1; }
__attribute__((noinline)) int parse_valid(Packet *p) { return p->checksum + 50; }

int main() {
    Packet pkt;
    klee_make_symbolic(&pkt, sizeof(pkt), "pkt");

    int result = 0;

    // Tag determines type (4 types)
    if (pkt.tag < 0x40) {
        // Type 1: len must be > 0 and < 64
        if (pkt.len == 0 || pkt.len >= 64) return parse_corrupt(&pkt);
        result = parse_type1(&pkt);
    } else if (pkt.tag < 0x80) {
        // Type 2: len must match tag's low nibble
        if ((pkt.len & 0x0F) != (pkt.tag & 0x0F)) return parse_corrupt(&pkt);
        result = parse_type2(&pkt);
    } else if (pkt.tag < 0xC0) {
        // Type 3: flags must be even
        if (pkt.flags & 0x01) return parse_corrupt(&pkt);
        result = parse_type3(&pkt);
    } else {
        // Type 4: flags must equal ~tag
        if (pkt.flags != (uint8_t)(~pkt.tag)) return parse_corrupt(&pkt);
        result = parse_type4(&pkt);
    }

    // Checksum validation across all types
    uint8_t expected = (pkt.tag ^ pkt.len ^ pkt.flags);
    if (pkt.checksum == expected) {
        result = parse_valid(&pkt);  // properly checksummed
    }

    // Deep validation for type 2 only
    if (pkt.tag >= 0x40 && pkt.tag < 0x80) {
        if (pkt.len > 16 && pkt.flags == 0xFF) {
            // Rare combination: type2 + long + all flags set
            result += 10000;
        }
    }

    return result;
}
