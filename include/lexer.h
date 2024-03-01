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
    TYPE_OPERATOR = 0,
    TYPE_VALUE,
    TYPE_OPEN_BRACKET,
    TYPE_CLOSE_BRACKET,
    TYPE_NONE
} Token_Type;

typedef struct {
    Token_Type type;
    union 
    {
        Value val;
        char op;
    };
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
    size_t tp;         // Token Pointer
} Lexer;


void print_token(Token tk);
void print_lex(Lexer *lex);
void lex_clean(Lexer *lex);
void lex_push(Lexer *lex, Token tk);

Token token_next(Lexer *lex);
Token_Type token_peek(Lexer *lex);

Value tokenise_value(String_View sv);
Lexer lexer(String_View src_sv, Var_List *vl);

#endif // LEXER_H_