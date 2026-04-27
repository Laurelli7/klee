/*
 * Structural Unit 17: Equality Chain (Sequential Equality Tests)
 * Source: equality_chain (131 functions across 7 programs)
 *         flvmeta AMF handlers (45 functions!), readelf symbol lookup,
 *         bison token comparison, nasm label matching
 * CFG: Sequence of `if (x == constant)` tests. Each test has a thin
 *      handler. If no match, falls through to next test.
 *      Different from SU02 (mixed comparisons) — here it's ALL equality.
 *      Different from switch (compiler may or may not lower to switch).
 * Hypothesis: Equality chain creates a linear search. Each branch is
 *   equally likely to match. Covnew should efficiently discover each match.
 *   But if the space is small enough, it may not discriminate.
 * Prediction: nurs:covnew (R4: each equality match covers different handler code)
 */
#include <klee/klee.h>

int lookup_keyword(unsigned char a, unsigned char b) {
    unsigned short key = ((unsigned short)a << 8) | b;

    /* Models a keyword hash table probe sequence  */
    if (key == 0x4966) return 100;  /* "If" */
    if (key == 0x446F) return 101;  /* "Do" */
    if (key == 0x466E) return 102;  /* "Fn" */
    if (key == 0x4F72) return 103;  /* "Or" */
    if (key == 0x4E6F) return 104;  /* "No" */
    if (key == 0x496E) return 105;  /* "In" */
    if (key == 0x5570) return 106;  /* "Up" */
    if (key == 0x4F6B) return 107;  /* "Ok" */
    if (key == 0x4869) return 108;  /* "Hi" */
    if (key == 0x4279) return 109;  /* "By" */
    if (key == 0x4173) return 110;  /* "As" */
    if (key == 0x4174) return 111;  /* "At" */
    if (key == 0x546F) return 112;  /* "To" */
    if (key == 0x4F6E) return 113;  /* "On" */
    if (key == 0x4765) return 114;  /* "Ge" */
    if (key == 0x4C74) return 115;  /* "Lt" */
    return 0;
}

int classify_token(unsigned char token_type) {
    if (token_type == 0x01) return 1;   /* identifier */
    if (token_type == 0x02) return 2;   /* number */
    if (token_type == 0x03) return 3;   /* string */
    if (token_type == 0x04) return 4;   /* operator */
    if (token_type == 0x05) return 5;   /* keyword */
    if (token_type == 0x10) return 10;  /* open_paren */
    if (token_type == 0x11) return 11;  /* close_paren */
    if (token_type == 0x20) return 20;  /* semicolon */
    if (token_type == 0x21) return 21;  /* comma */
    if (token_type == 0x30) return 30;  /* eof */
    if (token_type == 0xFF) return -1;  /* error */
    return 0;
}

int main() {
    unsigned char input[12];
    klee_make_symbolic(input, sizeof(input), "input");

    int total = 0;
    /* Process 4 tokens */
    for (int i = 0; i < 4; i++) {
        int type_cls = classify_token(input[i * 3]);
        if (type_cls == 5) {
            /* Keyword: look up the specific keyword */
            total += lookup_keyword(input[i * 3 + 1], input[i * 3 + 2]);
        } else if (type_cls > 0) {
            total += type_cls * 10 + input[i * 3 + 1];
        } else {
            total -= input[i * 3 + 2];
        }
    }
    return total;
}
