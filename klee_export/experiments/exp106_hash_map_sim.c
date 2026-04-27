// exp106: "Hash Map Sim" — Realistic hash table insert pattern.
// Symbolic keys are hashed (cheaply) to determine table slot.
// Different key combinations create different collision chains
// and coverage patterns.
//
// This amplifies the exp73 pattern: coverage depends on which
// constraint combination is explored (key collisions), not on
// explicit control flow visible in the CFG.
//
// 8 functions: insert_new, insert_collision, insert_full,
// lookup_found, lookup_clash, lookup_empty, delete_found, delete_miss
//
// BFS's systematic exploration creates diverse key combinations.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int insert_new(int acc, int slot) {
    return acc + slot * 11 + 100;
}
__attribute__((noinline)) int insert_collision(int acc, int slot, int depth) {
    return acc + slot * 13 + depth * 7 + 200;
}
__attribute__((noinline)) int insert_full(int acc) {
    return acc + 9999;
}
__attribute__((noinline)) int lookup_found(int acc, int key) {
    return acc + key * 5 + 300;
}
__attribute__((noinline)) int lookup_empty(int acc, int slot) {
    return acc + slot * 3 + 400;
}
__attribute__((noinline)) int lookup_wrong(int acc, int key, int found) {
    return acc + key * 2 + found + 500;
}
__attribute__((noinline)) int delete_found(int acc, int key) {
    return acc + key * 9 + 600;
}
__attribute__((noinline)) int delete_miss(int acc) {
    return acc + 700;
}

// Simple hash for slot selection
static inline uint8_t hash_key(uint8_t key, uint8_t table_size) {
    return ((key * 7 + 3) ^ (key >> 3)) % table_size;
}

int main() {
    uint8_t keys[5];   // 5 symbolic keys to insert
    uint8_t ops[3];    // 3 symbolic operations (lookup/delete)
    klee_make_symbolic(keys, sizeof(keys), "keys");
    klee_make_symbolic(ops, sizeof(ops), "ops");

    // Hash table: 6 slots, each stores (key, occupied)
    uint8_t table_keys[6] = {0};
    uint8_t table_occ[6] = {0};
    int acc = 0;

    // Insert 5 keys
    for (int i = 0; i < 5; i++) {
        uint8_t slot = hash_key(keys[i], 6);
        int probes = 0;

        // Linear probing
        while (probes < 6) {
            uint8_t s = (slot + probes) % 6;
            if (!table_occ[s]) {
                // Empty slot — insert
                table_keys[s] = keys[i];
                table_occ[s] = 1;
                acc = insert_new(acc, s);
                break;
            } else if (table_keys[s] == keys[i]) {
                // Duplicate key — collision
                acc = insert_collision(acc, s, probes);
                break;
            }
            probes++;
        }
        if (probes == 6) {
            acc = insert_full(acc);
        }
    }

    // 3 operations: lookup or delete based on ops byte
    for (int i = 0; i < 3; i++) {
        uint8_t key = ops[i];
        uint8_t slot = hash_key(key, 6);
        int probes = 0;

        while (probes < 6) {
            uint8_t s = (slot + probes) % 6;
            if (!table_occ[s]) {
                if (ops[i] & 0x80) {
                    acc = delete_miss(acc);
                } else {
                    acc = lookup_empty(acc, s);
                }
                break;
            } else if (table_keys[s] == key) {
                if (ops[i] & 0x80) {
                    acc = delete_found(acc, key);
                    table_occ[s] = 0;
                } else {
                    acc = lookup_found(acc, key);
                }
                break;
            } else {
                acc = lookup_wrong(acc, key, table_keys[s]);
            }
            probes++;
        }
    }

    return acc;
}
