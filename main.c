#define SV_IMPLEMENTAION
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "./sv.h"

typedef struct {
    char operator;
    int priority;
} Operator;

typedef union {
    int64_t INT;
    double FLOAT;
    Operator OPR;
} Object;

#define OBJ_INT(val) (Object) { .INT = (val) }
#define OBJ_FLOAT(val) (Object) { .FLOAT = (val) }

typedef enum {
    TYPE_INT = 0,
    TYPE_FLOAT,
    TYPE_OPERATOR,
    TYPE_OPEN_BRACKET,
    TYPE_CLOSE_BRACKET
} Token_Type;

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

typedef struct ast_node {
    Token token;
    struct ast_node *left_operand; 
    struct ast_node *right_operand; 
} Ast_Node;

typedef struct {
    Ast_Node *root;
    size_t count;
} Ast_Tree;

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

void expr_clean(Expresion *expr)
{
    free(expr->tokens);
    expr->count = 0;
    expr->capacity = 0;
}

void print_expr(Expresion *expr)
{
    for (size_t i = 0; i < expr->count; ++i) {
        switch (expr->tokens[i].type) {
        case TYPE_INT:
            printf("int: `%ld`\n", expr->tokens[i].value.INT);
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
            sv->data[i] == '('  ) 
            brk = 1;
        
        if (brk) break;
        i += 1;
    }

    if (i == 1) result.count = i;
    else result.count = i; 
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
        int temp = 0;
        int i = 0;
        int sign = 0;
        if (sv.data[0] == '-') {
            sign = 1;
            i++;
        }
        while (i < sv.count) {
            temp = temp + (sv.data[i] & 0x0F);
            temp = temp * 10;
            i++;
        }

        temp = temp / 10;
        if (sign == 1) temp = -temp;

        return OBJ_INT(temp);
    }
}

Expresion separate_src_by_tokens(String_View src)
{
    Expresion expr = {0};

    String_View src_sv = sv_trim(src);
    String_View src_trimed = { .count = src_sv.count }; 
    src_trimed.data = malloc((sizeof(src_sv.data[0]) * src_sv.count) + 2);

    memcpy(src_trimed.data, src_sv.data, src_trimed.count);
    memset(src_trimed.data + src_sv.count, '\0', sizeof('\0'));

    while (src_trimed.count > 0 && src_trimed.data[0] != '\0') {
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
                default:
                    fprintf(stderr, "Error: unknown operator `%c`\n",
                            expr->tokens[i].value.OPR.operator);
                    break;
            }
            opr_count += 1;
            if (bracket_count > 0) 
                expr->tokens[i].value.OPR.priority += bracket_count + opr_count;
        } else if (expr->tokens[i].type == TYPE_CLOSE_BRACKET) {
            bracket_count -= 1;
            if (opr_count > 0) opr_count -= 1; 
        } 
        else continue;
    }

    if (bracket_count != 0) {
        fprintf(stderr, "Error: cannot find close braket\n");
        exit(1);
    }
}

Ast_Node *ast_node_push(Ast_Node *node, Token tk)
{
    if (tk.type == TYPE_CLOSE_BRACKET || tk.type == TYPE_OPEN_BRACKET) {
        return node;
    }

    if (node == NULL) {
        node = malloc(sizeof(Ast_Node));
        node->token = tk; 
    } else {
        if (tk.type == TYPE_OPERATOR) {
            if (node->token.type == TYPE_INT || node->token.type == TYPE_FLOAT) {
                Token copy = node->token;
                node->token = tk;

                if (node->left_operand != NULL) node->right_operand = ast_node_push(node->right_operand, copy);
                else node->left_operand = ast_node_push(node->left_operand, copy);

            } else if (node->token.type == TYPE_OPERATOR) {
                if (node->token.value.OPR.priority > tk.value.OPR.priority) {
                    Ast_Node *new_node = malloc(sizeof(Ast_Node));
                    new_node->token = tk;
                    
                    if (new_node->left_operand != NULL) new_node->right_operand = node;        
                    else new_node->left_operand = node;
                    
                    return new_node;
                } else {
                    node->right_operand = ast_node_push(node->right_operand, tk);
                }
            }
        } else if (tk.type == TYPE_INT || tk.type == TYPE_FLOAT) {
            if (node->left_operand != NULL) node->right_operand = ast_node_push(node->right_operand, tk);
            else node->left_operand = ast_node_push(node->left_operand, tk);
        }
    }
    return node;
}

void ast_tree_push(Ast_Tree *ast, Token tk)
{
    ast->root = ast_node_push(ast->root, tk);
    ast->count += 1;
}

void generate_tree(Ast_Tree *ast, Expresion *expr)
{
    for (size_t i = 0; i < expr->count; ++i) 
        ast_tree_push(ast, expr->tokens[i]);
}

void print_tree(Ast_Node *node)
{
    if (!node) return;
    print_tree(node->left_operand);
    switch (node->token.type) {
        case TYPE_INT:
            printf("value: %ld\n", node->token.value.INT);
            break;
        case TYPE_FLOAT:
            printf("value: %lf\n", node->token.value.FLOAT);
            break;
        case TYPE_OPERATOR:
            printf("opr: %c, pr: %d\n",
                    node->token.value.OPR.operator, 
                    node->token.value.OPR.priority);
            break;
        default:
            break;
    }
    print_tree(node->right_operand);
}

int main(void)
{
    Ast_Tree ast = {0};
    
    Expresion expr = separate_src_by_tokens(sv_from_cstr (
        "(12831+12314)*(198*12387)/(745344-643546)"
    ));

    set_priorities(&expr);

    print_expr(&expr); 
    printf("\n---------------------------------\n\n");

    generate_tree(&ast, &expr);
    print_tree(ast.root);

    expr_clean(&expr);   
    return 0;
}