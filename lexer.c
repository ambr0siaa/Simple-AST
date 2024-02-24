#include "./lexer.h"

Value tokenise_value(String_View sv, int mode)
{
    int is_float;

    if (mode == NONE_MODE) is_float = sv_is_float(sv);
    else is_float = mode;

    if (is_float == 1) {
        char *float_cstr = malloc(sizeof(char) * sv.count + 1);
        char *endptr = float_cstr; 
        memcpy(float_cstr, sv.data, sv.count);

        double d = strtod(float_cstr, &float_cstr);

        if (d == 0 && endptr == float_cstr) {
            fprintf(stderr, "Error: cannot parse `%s` to float64\n",float_cstr);
            EXIT;
        }

        return VALUE_FLOAT(d);

    } else {
        int64_t value = sv_to_int(sv);
        return VALUE_INT(value);
    }
}

void lex_push(Lexer *lex, Token tk)
{
    if (lex->capacity == 0) {
        lex->capacity = LEX_INIT_CAPACITY;
        lex->tokens = malloc(LEX_INIT_CAPACITY * sizeof(lex->tokens[0]));
        lex->tp = 0;
    }

    if (lex->count + 1 > lex->capacity) {
        lex->capacity *= 2;
        lex->tokens = realloc(lex->tokens, lex->capacity * sizeof(lex->tokens[0])); 
    }

    lex->tokens[lex->count++] = tk;
}

void lex_clean(Lexer *lex)
{
    free(lex->tokens);
    lex->count = 0;
    lex->capacity = 0;
}

String_View lex_sep_by_operator(String_View *sv)
{
    size_t i = 0;
    int brk = 0;
    String_View result;

    while (i < sv->count) {
        if (sv->data[i] == '+' ||
            sv->data[i] == '-' ||
            sv->data[i] == '*' ||
            sv->data[i] == '/' ||
            sv->data[i] == ')' ||
            sv->data[i] == '('  ) 
            brk = 1;
        
        if (brk) break;
        i += 1;
    }

    result.count = i;
    result.data = sv->data;

    sv->count -= i;
    sv->data += i;

    return result;
}

Lexer lexer(String_View src)
{
    Lexer lex = {0};

    while (src.count != 0) {
        Token tk;
        if (isdigit(src.data[0])) {
            String_View value = sv_trim(lex_sep_by_operator(&src));   
            tk.type = TYPE_VALUE;

            if (sv_is_float(value)) {
                tk.val = tokenise_value(value, FLOAT_MODE);
            } else {
                tk.val = tokenise_value(value, INT_MODE);
            }
        } else {
            if (src.count != 0) {
                String_View opr = sv_trim(sv_div_by_next_symbol(&src));    
                switch (opr.data[0]) {
                    case '(': tk.type = TYPE_OPEN_BRACKET;  break;
                    case ')': tk.type = TYPE_CLOSE_BRACKET; break;

                    case '/': tk.type = TYPE_OPERATOR; tk.op.type = OP_DIV;     break;
                    case '+': tk.type = TYPE_OPERATOR; tk.op.type = OP_PLUS;    break;
                    case '*': tk.type = TYPE_OPERATOR; tk.op.type = OP_MULT;    break;
                    case '-': tk.type = TYPE_OPERATOR; tk.op.type = OP_MINUS;   break;

                    default:
                        fprintf(stderr, "Error: unknown operator `%c`\n", opr.data[0]);
                        EXIT;
                }
                tk.op.operator = opr.data[0];
            }
        }
        lex_push(&lex, tk);
    }

    return lex;
}

Token token_next(Lexer *lex)
{
    if (lex->tp >= lex->count) {
        return (Token) { .type = TYPE_NONE };
    } else {
        Token tk = lex->tokens[lex->tp];
        lex->tp += 1;
        return tk;
    }
}

Token_Type token_peek(Lexer *lex)
{
    if (lex->tp >= lex->count) {
        return TYPE_NONE;
    } else {
        Token_Type type = lex->tokens[lex->tp].type;
        return type;
    }
}

void print_token(Token tk)
{
    switch (tk.type) {
        case TYPE_VALUE: {
            if (tk.val.type == VAL_FLOAT) {
                printf("float: `%lf`\n", tk.val.f64);
            } else {
                printf("int: `%ld`\n", tk.val.i64);
            }
            break;
        }
        case TYPE_OPERATOR: {
            printf("opr: `%c`\n", tk.op.operator);
            break;
        }
        case TYPE_OPEN_BRACKET: {
            printf("open bracket: `%c`\n", tk.op.operator);
            break;
        }
        case TYPE_CLOSE_BRACKET: {
            printf("close bracket: `%c`\n", tk.op.operator);
            break;
        }
        case TYPE_NONE:
        default: 
            fprintf(stderr, "Error: unknown type `%u`\n", tk.type);
            EXIT;
    }
}

void print_lex(Lexer *lex)
{
    printf("\n-------------- LEXER --------------\n\n");
    for (size_t i = 0; i < lex->count; ++i) {
        print_token(lex->tokens[i]);
    }
    printf("\n-----------------------------------\n\n");
}