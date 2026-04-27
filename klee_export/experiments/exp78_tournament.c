// exp78: "Early Exit Tournament" — N states compete; the FIRST to
// reach an exit terminates (simulating a race condition or first-match).
// Tests which searcher finds the earliest exit fastest.
//
// Structure: 8 "competitors," each behind a different symbolic byte.
// Competitor i exits iff byte[i] == magic[i]. The competitors are
// checked in sequence, so DFS will always check competitor 0 first.
//
// After the tournament, noise branches amplify the state count
// for the "no winner" path.
//
// DFS: checks competitor 0 first, finds it or moves to 1, etc.
//      Linear through the tournament.
// BFS: checks all competitors at depth 1, finding all winners.
// random-path: randomly hits one competitor.
// covnew: each competitor exit is unique code → chases exits.
//
// The trick: competitor 7 has the MOST unique code after its exit
// (calls 4 unique functions). covnew should chase it. But reaching
// competitor 7 requires passing through 7 "no match" forks first.
//
// Expected: DFS finds competitor 0 quickest. covnew finds
// competitor 7 quickest (most novel post-exit code).
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int winner_0(int x) { return x + 100; }
__attribute__((noinline)) int winner_1(int x) { return x + 200; }
__attribute__((noinline)) int winner_2(int x) { return x + 300; }
__attribute__((noinline)) int winner_3(int x) { return x + 400; }
__attribute__((noinline)) int winner_4(int x) { return x + 500; }
__attribute__((noinline)) int winner_5(int x) { return x + 600; }
__attribute__((noinline)) int winner_6(int x) { return x + 700; }
// Winner 7: richest post-exit code
__attribute__((noinline)) int winner_7a(int x) { return x + 800; }
__attribute__((noinline)) int winner_7b(int x) { return x + 900; }
__attribute__((noinline)) int winner_7c(int x) { return x + 1000; }
__attribute__((noinline)) int winner_7d(int x) { return x + 1100; }

__attribute__((noinline)) int no_winner(int x) { return x + 1; }

int main() {
    uint8_t contestants[8];
    uint8_t noise[3];
    klee_make_symbolic(contestants, sizeof(contestants), "cont");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;

    // Noise preamble
    for (int i = 0; i < 3; i++) {
        if (noise[i] & 0x01) result++;
        if (noise[i] & 0x02) result++;
        if (noise[i] & 0x04) result++;
        if (noise[i] & 0x08) result++;
        if (noise[i] & 0x10) result++;
        if (noise[i] & 0x20) result++;
        if (noise[i] & 0x40) result++;
        if (noise[i] & 0x80) result++;
    }

    // Tournament: first match wins
    if (contestants[0] == 0x10) { return winner_0(result); }
    if (contestants[1] == 0x20) { return winner_1(result); }
    if (contestants[2] == 0x30) { return winner_2(result); }
    if (contestants[3] == 0x40) { return winner_3(result); }
    if (contestants[4] == 0x50) { return winner_4(result); }
    if (contestants[5] == 0x60) { return winner_5(result); }
    if (contestants[6] == 0x70) { return winner_6(result); }
    if (contestants[7] == 0x80) {
        // Richest winner: 4 unique functions
        result = winner_7a(result);
        result = winner_7b(result);
        result = winner_7c(result);
        result = winner_7d(result);
        return result;
    }

    // No winner: all 8 contestants lost
    return no_winner(result);
}
