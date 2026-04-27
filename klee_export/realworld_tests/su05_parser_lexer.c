/*
 * Structural Unit 5: Parser Lexer Loop
 * Source pattern: bc/yylex (153 BBs), bison/gram_lex (905 BBs!), lua/llex (105 BBs),
 *                 nasm/do_directive (530 BBs) — character-level lexing
 * CFG shape: Loop reads characters; multi-way branch classifies char class;
 *            state transitions per token type. Deep chain inside a loop.
 * Rule prediction: R1+R9 → nurs:covnew (lexer = deep chain inside byte loop)
 */
#include <klee/klee.h>

#define INPUT_LEN 16

enum token_type { T_EOF, T_NUM, T_ID, T_OP, T_STRING, T_LPAREN, T_RPAREN, T_ERROR };

struct token {
    enum token_type type;
    int value;
};

int pos;
unsigned char *source;
int source_len;

static unsigned char peek(void) {
    if (pos >= source_len) return 0;
    return source[pos];
}

static unsigned char advance(void) {
    if (pos >= source_len) return 0;
    return source[pos++];
}

static int is_digit(unsigned char c) { return c >= '0' && c <= '9'; }
static int is_alpha(unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static int is_space(unsigned char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

struct token lex_token(void) {
    struct token t;

    /* Skip whitespace */
    while (pos < source_len && is_space(peek())) {
        advance();
    }

    if (pos >= source_len) {
        t.type = T_EOF;
        t.value = 0;
        return t;
    }

    unsigned char c = peek();

    /* Number literal */
    if (is_digit(c)) {
        t.type = T_NUM;
        t.value = 0;
        while (pos < source_len && is_digit(peek())) {
            t.value = t.value * 10 + (advance() - '0');
            if (t.value > 9999) { t.type = T_ERROR; return t; }
        }
        /* Check for hex suffix */
        if (peek() == 'x' || peek() == 'X') {
            advance();
            t.value |= 0x10000;
        }
        return t;
    }

    /* Identifier */
    if (is_alpha(c)) {
        t.type = T_ID;
        t.value = 0;
        int len = 0;
        while (pos < source_len && (is_alpha(peek()) || is_digit(peek()))) {
            t.value = t.value * 31 + advance();
            len++;
            if (len > 8) break;
        }
        return t;
    }

    /* Operators and punctuation */
    advance();
    switch (c) {
        case '+': t.type = T_OP; t.value = 1;
            if (peek() == '+') { advance(); t.value = 10; }  /* ++ */
            else if (peek() == '=') { advance(); t.value = 11; } /* += */
            return t;
        case '-': t.type = T_OP; t.value = 2;
            if (peek() == '-') { advance(); t.value = 20; }
            else if (peek() == '=') { advance(); t.value = 21; }
            else if (peek() == '>') { advance(); t.value = 22; }
            return t;
        case '*': t.type = T_OP; t.value = 3;
            if (peek() == '=') { advance(); t.value = 31; }
            return t;
        case '/': t.type = T_OP; t.value = 4;
            if (peek() == '=') { advance(); t.value = 41; }
            else if (peek() == '/') {
                /* Line comment — skip to end */
                while (pos < source_len && peek() != '\n') advance();
                return lex_token(); /* recurse to get next real token */
            }
            return t;
        case '(': t.type = T_LPAREN; t.value = 0; return t;
        case ')': t.type = T_RPAREN; t.value = 0; return t;
        case '=': t.type = T_OP; t.value = 5;
            if (peek() == '=') { advance(); t.value = 50; }
            return t;
        case '!': t.type = T_OP; t.value = 6;
            if (peek() == '=') { advance(); t.value = 60; }
            return t;
        case '<': t.type = T_OP; t.value = 7;
            if (peek() == '=') { advance(); t.value = 70; }
            else if (peek() == '<') { advance(); t.value = 71; }
            return t;
        case '>': t.type = T_OP; t.value = 8;
            if (peek() == '=') { advance(); t.value = 80; }
            else if (peek() == '>') { advance(); t.value = 81; }
            return t;
        case '"': /* String literal */
            t.type = T_STRING;
            t.value = 0;
            while (pos < source_len && peek() != '"') {
                unsigned char sc = advance();
                if (sc == '\\' && pos < source_len) {
                    advance(); /* skip escaped char */
                }
                t.value++;
            }
            if (pos < source_len) advance(); /* consume closing quote */
            return t;
        default:
            t.type = T_ERROR;
            t.value = c;
            return t;
    }
}

int main() {
    unsigned char input[INPUT_LEN];
    klee_make_symbolic(input, sizeof(input), "input");

    source = input;
    source_len = INPUT_LEN;
    pos = 0;

    int token_count = 0;
    int total_value = 0;

    for (int i = 0; i < 8; i++) {
        struct token t = lex_token();
        if (t.type == T_EOF || t.type == T_ERROR) break;
        token_count++;
        total_value += t.value;
    }

    return token_count * 100 + (total_value & 0xFF);
}
