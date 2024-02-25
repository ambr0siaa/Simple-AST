#ifndef LEXER_H_
#define LEXER_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "./sv.h"
#include "./var.h"

#define EXIT exit(1)

typedef enum {
    OP_PLUS = 0,
    OP_MINUS,
    OP_MULT,
    OP_DIV,
    OP_MOD,
    OP_NONE
} Operator_Type;

typedef struct {
    Operator_Type type;
    char operator;
} Operator;

typedef enum {
    TYPE_OPERATOR = 0,
    TYPE_VALUE,
    TYPE_OPEN_BRACKET,
    TYPE_CLOSE_BRACKET,
    TYPE_NONE
} Token_Type;

typedef struct {
    Token_Type type;
    union {
        Value val;
        Operator op;
    };
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
    size_t tp;         // Token Pointer
} Lexer;

// It needs for to don't call `sv_is_float` one more time if it was call later
#define NONE_MODE 0
#define FLOAT_MODE 1
#define INT_MODE 2

Value tokenise_value(String_View sv, int mode);

void print_token(Token tk);
void print_lex(Lexer *lex);
void lex_clean(Lexer *lex);
void lex_push(Lexer *lex, Token tk);

Token token_next(Lexer *lex);
Token_Type token_peek(Lexer *lex);

Lexer lexer(String_View src_sv, Var_List *vl);

#endif // LEXER_H_