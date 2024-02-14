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
} Ast;

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
            printf("operator: `%c`, priority: %d\n", 
                    expr->tokens[i].value.OPR.operator, 
                    expr->tokens[i].value.OPR.priority);
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
        int64_t value = sv_to_int(sv);
        return OBJ_INT(value);
    }
}

Expresion separate_src_by_tokens(String_View src)
{
    Expresion expr = {0};
    String_View src_trimed = sv_trim(src);

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
                case '(': tk.type = TYPE_OPEN_BRACKET;  break;
                case ')': tk.type = TYPE_CLOSE_BRACKET; break;
                case '+': tk.type = TYPE_OPERATOR;      break;
                case '*': tk.type = TYPE_OPERATOR;      break;
                case '-': tk.type = TYPE_OPERATOR;      break;
                case '/': tk.type = TYPE_OPERATOR;      break;
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


// TODO: remake set_priorities
void set_priorities(Expresion *expr)
{
    size_t bracket_count = 0;
    size_t opr_count = 0;
    size_t zero_priority_count = 0;
    
    Operator *prev;
    Operator *cur;

    for (int i = expr->count; i >= 0; --i) {
        if (expr->tokens[i].type == TYPE_CLOSE_BRACKET) {
            bracket_count += 1;           
        } else if (expr->tokens[i].type == TYPE_OPERATOR) {
            prev = cur;
            
            switch (expr->tokens[i].value.OPR.operator) {
                case '+': expr->tokens[i].value.OPR.priority = 0; 
                        if (bracket_count == 0) {
                            expr->tokens[i].value.OPR.priority += zero_priority_count;
                            zero_priority_count++;
                        }
                        break;
                case '-': expr->tokens[i].value.OPR.priority = 0;
                        if (bracket_count == 0) {
                            expr->tokens[i].value.OPR.priority += zero_priority_count;
                            zero_priority_count++;
                        }
                        break;
                case '*': expr->tokens[i].value.OPR.priority = 1; 
                          opr_count -= 1; break;
                case '/': expr->tokens[i].value.OPR.priority = 1; 
                          opr_count -= 1; break;
                default:
                    fprintf(stderr, "Error: unknown operator `%c`. exit with 1\n",
                            expr->tokens[i].value.OPR.operator);
                    break;
            }
            opr_count += 1;
            if (bracket_count > 0) expr->tokens[i].value.OPR.priority += bracket_count + opr_count;

            cur = &expr->tokens[i].value.OPR;

            if (bracket_count == 0 ) {
                if ((cur->operator == '+' || cur->operator == '-') && 
                    (prev->operator == '*' || prev->operator == '/')) {
                    if (cur->priority >= prev->priority) {
                        prev->priority += cur->priority;
                    }      
                } else if ((prev->operator == '+' || prev->operator == '-') && 
                            (cur->operator == '*' || cur->operator == '/')) {
                    if (cur->priority <= prev->priority) {
                        cur->priority += prev->priority;
                    }
                } else if (cur->operator == '*' || cur->operator == '/' &&
                           prev->operator == '*' || prev->operator == '/') {
                    if (cur->priority <= prev->priority) {
                        cur->priority += prev->priority + 1;
                    }
                }
            } 

        } else if (expr->tokens[i].type == TYPE_OPEN_BRACKET) 
            bracket_count -= 1;
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

void Ast_push(Ast *ast, Token tk)
{
    ast->root = ast_node_push(ast->root, tk);
    ast->count += 1;
}

void generate_tree(Ast *ast, Expresion *expr)
{
    for (size_t i = 0; i < expr->count; ++i) 
        Ast_push(ast, expr->tokens[i]);
}

#define TAB(iter) ({                    \
    for (int j = 0; j < (iter); ++j) {  \
        printf(" ");                    \
    }                                   \
    (iter)++;                           \
})                                     

void print_tree(Ast_Node *node)
{
    static int i;

    TAB(i);
    switch (node->token.type) {
        case TYPE_INT:
            printf("value: '%ld'\n", node->token.value.INT);
            break;
        case TYPE_FLOAT:
            printf("value: '%lf'\n", node->token.value.FLOAT);
            break;
        case TYPE_OPERATOR:
            printf("opr: '%c'\n", node->token.value.OPR.operator );
            break;
        default:
            break;
    }

    if (node->right_operand != NULL) {
        TAB(i);
        printf("right: ");
        print_tree(node->right_operand);
        i--;
    }

    if (node->left_operand != NULL) {
        TAB(i);
        printf("left: ");
        print_tree(node->left_operand);
        i--;
    }

    i--;
}

#define DO_OP(dst, operator, op1, op2, type)   \
    do {                                       \
        if (type == 'f') {                                      \
            (dst)->token.value.FLOAT = (op1)->token.value.FLOAT operator (op2)->token.value.FLOAT;          \
        } else if (type == 'i'){                                                                                        \
            (dst)->token.value.INT = (op1)->token.value.INT operator (op2)->token.value.INT;          \
        }                                                                                           \
    } while(0)              

Ast_Node *resolve_root(Ast_Node *node)
{   
    if (node->left_operand != NULL && node->right_operand != NULL) {
        if (node->right_operand->token.type == TYPE_OPERATOR ||
            node->left_operand->token.type == TYPE_OPERATOR ) {
                node->left_operand = resolve_root(node->left_operand);
                node->right_operand = resolve_root(node->right_operand);
        }
            
        if (node->token.type == TYPE_OPERATOR) {
            char type;
            if (node->left_operand->token.type == TYPE_FLOAT) { 
                type = 'f';
                node->token.type = TYPE_FLOAT;
            } else {
                type = 'i';
                node->token.type = TYPE_INT;
            }

            switch (node->token.value.OPR.operator) {
                case '+':
                    DO_OP(node, +, node->left_operand, node->right_operand, type);
                    break;
                case '*':
                    DO_OP(node, *, node->left_operand, node->right_operand, type);
                    break;
                case '-':
                    DO_OP(node, -, node->left_operand, node->right_operand, type);
                    break;
                case '/':
                    DO_OP(node, /, node->left_operand, node->right_operand, type);
                    break;
                default:
                    fprintf(stderr, "Error, unknown operator `%c`\n", node->token.value.OPR.operator);
                    exit(1);
            }

            free(node->left_operand);
            free(node->right_operand);
            return node;
        }
    }
    return node;
}

void resolve_ast(Ast *ast)
{
    ast->root = resolve_root(ast->root);
    ast->count = 1; 
}

void ast_clean(Ast *ast)
{
    // TODO: Implement this shit
}

int main(void)
{
    Ast ast = {0};

    char *test1 = "(1 * ((1 + 1) + (1 + 1)) / 1 / 1) * 1 * 1 / 1 - 1 * (1 * 1 + 1 * 1) * 1 + 1 * (1 * (1 + 1) * 1) * 1 * 1 * 1 * 1 * 1 * 1 * 1 * 1 - ( 1 * (1 + 1) * 1) * 1 + 1 + 1 * 1 * 1 - 1";
    char *test2 = "(2.0 / 4.0) * 2.0 + (1.0 / 2.0 + 0.001)";

    Expresion expr = separate_src_by_tokens(sv_from_cstr(test2));
    set_priorities(&expr);

    // print_expr(&expr);
    generate_tree(&ast, &expr);
    // print_tree(ast.root);

    resolve_ast(&ast);
    printf("%lf\n", ast.root->token.value.FLOAT);

    free(ast.root);
    expr_clean(&expr);   
    return 0;
}