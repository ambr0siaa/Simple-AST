#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

#define NONE_OPERATOR 'N'
#define PLUS_OPERATOR '+'
#define MINUS_OPERATOR '-'
#define MULT_OPERATOR '*'
#define DIV_OPERATOR '/'

typedef struct {
    char *data;
    int count;
} String_View;

#define SV_Fmt "%.*s"
#define SV_Args(sv) (int) (sv).count, (sv).data

String_View sv_from_cstr(char *cstr)
{
    return (String_View) {
        .count = strlen(cstr),
        .data = cstr
    };
}

String_View sv_trim_left(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return (String_View) {
        .count = sv.count - i,
        .data = sv.data + i
    };
}

String_View sv_trim_right(String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - i - 1])) {
        i += 1;
    }

    return (String_View) {
        .count = sv.count - i,
        .data = sv.data
    };
}

String_View sv_trim(String_View sv)
{
    return sv_trim_right(sv_trim_left(sv));
}

String_View sv_div_by_delim(String_View *sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    String_View result = {
        .count = i,
        .data = sv->data
    };

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data += i + 1;
    } else {
        sv->count -= i;
        sv->data += i;
    }

    return result;
}

int sv_cmp(String_View sv1, String_View sv2)
{
    if (sv1.count != sv2.count) {
        return 0;
    } else {
        return memcmp(sv1.data, sv2.data, sv1.count) == 0;
    }
}

int sv_to_int(String_View sv)
{
    int result = 0;
    for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); ++i) {
        result = result * 10 + sv.data[i] - '0'; 
    }
    
    return result;
}

int sv_is_float(String_View sv)
{
    int res = 0;
    for (size_t i = 0; i < sv.count; ++i) {
        if (sv.data[i] == '.') {
            res = 1;
            break;
        }
    }
    return res;
}

String_View sv_div_by_next_symbol(String_View *sv)
{
    String_View result;

    size_t i = 0;
    size_t count = 0;
    while (i < sv->count) {
        if (!isspace(sv->data[i])) {
            count++;
        }

        if (count == 2) { 
            break;
        }

        ++i;
    }

    if (i == 1) result.count = i;
    else result.count = (i - 1); 

    result.data = sv->data;
    
    if (i < sv->count) {
        sv->count -= (i - 1);
        sv->data += i;
    } else {
        sv->count -= i;
        sv->data += i;
    }

    return result;
}

typedef struct {
    char operator;
    int priority;
} Operator;

typedef union {
    int INT;
    double FLOAT;
    Operator OPR;
} Object;

#define OBJ_INT(val) (Object) { .INT = (val) }
#define OBJ_FLOAT(val) (Object) { .FLOAT = (val) }

typedef struct ast_node {
    int value;
    struct ast_node *left_operand; 
    struct ast_node *right_operand; 
} Ast_Node;

typedef struct {
    Ast_Node *root;
    size_t count;
} Ast_Tree;

typedef enum {
    TYPE_INT = 0,
    TYPE_FLOAT,
    TYPE_OPERATOR,
    TYPE_OPEN_BRACKET,
    TYPE_CLOSE_BRACKET
} Token_Type;

// mark:
//  0 - none
//  1 - int
//  2 - float
//  3 - operator
//  4 - open
typedef struct {
    Token_Type type;
    Object value;
} Token;

#define EXPR_INIT_CAPACITY 123

typedef struct {
    Token *tokens;
    size_t capacity;
    size_t count;
} Expresion;

void token_push(Expresion *expr, Token tk)
{
    if (expr->capacity == 0) {
        expr->capacity = EXPR_INIT_CAPACITY;
        expr->tokens = malloc(EXPR_INIT_CAPACITY * sizeof(expr->tokens[0]));
    }

    if (expr->count + 1 > expr->capacity) {
        expr->capacity *= 2;
        expr->tokens = realloc(expr->tokens, expr->capacity); 
    }

    expr->tokens[expr->count++] = tk;
}

Token token_pop(Expresion *expr)
{
    if (expr->count > 0) {
        Token tk = expr->tokens[--expr->count];
        return tk;
    }
}

String_View separate_by_operator(String_View *sv)
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
            sv->data[i] == '('  ) {
                brk = 1;
        }
        
        if (brk) break;
        i += 1;
    }

    if (i == 1) result.count = i;
    else result.count = i - 1; 
    result.data = sv->data;
    
    if (i < sv->count) {
        int t = i - 1;
        sv->count -= t;
        sv->data += i;
    } else {
        sv->count -= i;
        sv->data += i;
    }

    return result;
}

// 0 - none
// 1 - float
// 2 - int
Object parse_value(String_View sv, int value_kind)
{
    int is_float;

    if (value_kind == 0) is_float = sv_is_float(sv);
    else is_float = value_kind;

    if (is_float == 1) {
        char *float_cstr = malloc(sizeof(char) * sv.count + 1);
        memcpy(float_cstr, sv.data, sv.count);

        char *endptr = float_cstr; 

        double d = strtod(float_cstr, &float_cstr);

        if (d == 0 && endptr == float_cstr) {
            fprintf(stderr, "Error: cannot parse `%s` to float64\n",float_cstr);
            exit(1);
        }

        return OBJ_FLOAT(d);

    } else {
        // TODO: parse int
        uint64_t INT = sv_to_int(sv);
        return OBJ_INT(INT);
    }
}

void sv_cut_space_left(String_View *sv)
{
    size_t i = 0;
    while(i < sv->count && isspace(sv->data[i])) {
        ++i;
    }

    if (i != sv->count) {
        int t = i - 1;
        sv->count -= t;
        sv->data += i;
    }
}

void sv_cut_space_right(String_View *sv)
{
    size_t i = 0;
    while(i < sv->count && isspace(sv->data[sv->count - i])) {
        ++i;
    }

    if (i != sv->count) {
        int t = i - 1;
        sv->count -= t;
    }
}

void sv_cut_space(String_View *sv) 
{
    sv_cut_space_left(sv);
    sv_cut_space_right(sv);
}

Expresion separate_src_by_tokens(String_View src)
{
    Expresion expr = {0};
    String_View src_trimed = sv_trim(src);
    while (src_trimed.count > 0 && (int*)(src_trimed.data[0]) != NULL) {
        Token tk;
        if (isdigit(src_trimed.data[0])) {
            String_View value = sv_trim(separate_by_operator(&src_trimed));
            if (sv_is_float(value)) {
                tk.type = TYPE_FLOAT;
                tk.value = parse_value(value, 1);
            } else {
                tk.type = TYPE_INT;
                tk.value = parse_value(value, 2);
            }
        } else {
            String_View opr = sv_trim(sv_div_by_next_symbol(&src_trimed));
            switch (opr.data[0]) {
                case '(':
                    tk.type = TYPE_OPEN_BRACKET;
                    break;
                case ')':
                    tk.type = TYPE_CLOSE_BRACKET;
                    break;
                case '+':
                    tk.type = TYPE_OPERATOR;
                    break;
                case '*':
                    tk.type = TYPE_OPERATOR;
                    break;
                case '-':
                    tk.type = TYPE_OPERATOR;
                    break;
                case '/':
                    tk.type = TYPE_OPERATOR;
                    break;
                default:
                    fprintf(stderr, "Error: unknown operator `%c`\n", opr.data[0]);
                    exit(1);
            }
            tk.value.OPR.operator = opr.data[0];
        }
        token_push(&expr, tk);
    }
    return expr;
}

void print_expr(Expresion *expr)
{
    for (size_t i = 0; i < expr->count; ++i) {
        switch (expr->tokens[i].type) {
        case TYPE_INT:
            printf("int: `%d`\n", expr->tokens[i].value.INT);
            break;
        case TYPE_FLOAT:
            printf("float: `%lf`\n", expr->tokens[i].value.FLOAT);
            break;
        case TYPE_OPERATOR:
            printf("operator: `%c`, priority: %d\n", expr->tokens[i].value.OPR.operator, expr->tokens[i].value.OPR.priority);
            break;
        case TYPE_OPEN_BRACKET:
            printf("open bracket: `%c`\n", expr->tokens[i].value.OPR.operator);
            break;
        case TYPE_CLOSE_BRACKET:
            printf("close bracket: `%c`\n", expr->tokens[i].value.OPR.operator);
            break;
        default:
            fprintf(stderr, "Error: unknown type `%u`\n", expr->tokens[i].type);
            exit(1);
        }
    }
}

void set_priorities(Expresion *expr)
{
    size_t bracket_count = 0;
    size_t opr_count = 0;
    for (size_t i = 0; i < expr->count; ++i) {
        if (expr->tokens[i].type == TYPE_OPEN_BRACKET) {
            bracket_count += 1;           
        } else if (expr->tokens[i].type == TYPE_OPERATOR) {
            switch (expr->tokens[i].value.OPR.operator) {
            case '+':
                expr->tokens[i].value.OPR.priority = 0;
                break;
            case '-':
                expr->tokens[i].value.OPR.priority = 0;
                break;
            case '*':
                expr->tokens[i].value.OPR.priority = 1;
                break;
            case '/':
                expr->tokens[i].value.OPR.priority = 1;
                break;
            }
            opr_count += 1;
            if (bracket_count > 0) expr->tokens[i].value.OPR.priority += bracket_count + opr_count;
        } else if (expr->tokens[i].type == TYPE_CLOSE_BRACKET) {
            bracket_count -= 1;
            if (opr_count > 0) opr_count -= 1; 
        } else {
            continue;
        }
    }

    if (bracket_count != 0) {
        fprintf(stderr, "Error: cannot find close braket\n");
        exit(1);
    }
}

int main(void)
{
    String_View sv = sv_from_cstr("((1 + 2) * 2) * 5 + (5 * (5 + 1))");
    Expresion expr = separate_src_by_tokens(sv);
    print_expr(&expr);
    set_priorities(&expr);

    printf("\n---------------------------------\n\n");

    print_expr(&expr);    
    return 0;
}