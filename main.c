#define SV_IMPLEMENTAION
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "./sv.h"

typedef enum {
    OP_PLUS = 0,
    OP_MINUS,
    OP_MULT,
    OP_DIV,
    OP_NONE
} Operator_Type;

typedef struct {
    Operator_Type type;
    char operator;
} Operator;

typedef enum {
    FLOAT = 0,
    INT
} Value_Type;

typedef struct {
    Value_Type type;
    union {
        int64_t i64;
        double f64;
    };
} Value;

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
    Token *tokens;
    size_t count;
    size_t capacity;
} Token_List;

typedef struct ast_node {
    Token token;
    struct ast_node *left_operand; 
    struct ast_node *right_operand; 
} Ast_Node;

typedef struct {
    Ast_Node *root;
    size_t count;
} Ast;

#define VAL_INT(val) (Value) { .type = INT, .i64 = (val) }
#define VAL_FLOAT(val) (Value) { .type = FLOAT, .f64 = (val) }

// 0 - none
// 1 - float
// 2 - int
Value parse_value(String_View sv, int value_kind)
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

        return VAL_FLOAT(d);

    } else {
        int64_t value = sv_to_int(sv);
        return VAL_INT(value);
    }
}

#define TL_INIT_CAPACITY 128

void token_push(Token_List *tl, Token tk)
{
    if (tl->capacity == 0) {
        tl->capacity = TL_INIT_CAPACITY;
        tl->tokens = malloc(TL_INIT_CAPACITY * sizeof(tl->tokens[0]));
    }

    if (tl->count + 1 > tl->capacity) {
        tl->capacity *= 2;
        tl->tokens = realloc(tl->tokens, tl->capacity * sizeof(tl->tokens[0])); 
    }

    tl->tokens[tl->count++] = tk;
}

void tl_clean(Token_List *tl)
{
    free(tl->tokens);
    tl->count = 0;
    tl->capacity = 0;
}

Token_List tokensizer(String_View src)
{
    Token_List tl = {0};
    String_View src_trimed = sv_trim(src);

    while (src_trimed.count > 0 && src_trimed.data[0] != '\0') {
        Token tk;
        if (isdigit(src_trimed.data[0])) {
            String_View value = sv_trim(separate_by_operator(&src_trimed));   
            tk.type = TYPE_VALUE;
            if (sv_is_float(value)) {
                tk.val = parse_value(value, 1);
            } else {
                tk.val = parse_value(value, 2);
            }
        } else {
            if (src_trimed.count != 0) {
                String_View opr = sv_trim(sv_div_by_next_symbol(&src_trimed));    
                switch (opr.data[0]) {
                    case '(': tk.type = TYPE_OPEN_BRACKET;  break;
                    case ')': tk.type = TYPE_CLOSE_BRACKET; break;

                    case '/': tk.type = TYPE_OPERATOR; tk.op.type = OP_DIV;     break;
                    case '+': tk.type = TYPE_OPERATOR; tk.op.type = OP_PLUS;    break;
                    case '*': tk.type = TYPE_OPERATOR; tk.op.type = OP_MULT;    break;
                    case '-': tk.type = TYPE_OPERATOR; tk.op.type = OP_MINUS;   break;

                    default:
                        fprintf(stderr, "Error: unknown operator `%c`\n", opr.data[0]);
                        exit(1);
                }
                tk.op.operator = opr.data[0];
            }
        }
        token_push(&tl, tk);
    }
    return tl;
}

#define TAB(iter) ({                    \
    for (int j = 0; j < (iter); ++j) {  \
        printf(" ");                    \
    }                                   \
    (iter)++;                           \
})                                     

void print_node(Ast_Node *node)
{
    switch (node->token.type) {
        case TYPE_VALUE:
            if (node->token.val.type == FLOAT) printf("value: '%lf'\n", node->token.val.f64);
            else printf("value: '%ld'\n", node->token.val.i64);
            break;
        case TYPE_OPERATOR:
            printf("opr: '%c'\n", node->token.op.operator);
            break;
        default:
            break;
    }
}

void print_tree(Ast_Node *node)
{
    static int i;

    TAB(i);
    print_node(node);

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

#define BINARY_OP(dst, operator, op1, op2, type)                                        \
    do {                                                                                \
        if (type == 'f') {                                                              \
            (dst)->token.val.f64 = (op1)->token.val.f64 operator (op2)->token.val.f64;  \
        } else if (type == 'i') {                                                       \
            (dst)->token.val.i64 = (op1)->token.val.i64 operator (op2)->token.val.i64;  \
        }                                                                               \
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
            if (node->left_operand->token.val.type == FLOAT) {
                type = 'f';
                node->token.val.type = FLOAT;
            } else { 
                type = 'i';
                node->token.val.type = INT;
            }
            
            node->token.type = TYPE_VALUE;

            switch (node->token.op.operator) {
                case '+': BINARY_OP(node, +, node->left_operand, node->right_operand, type); break;
                case '*': BINARY_OP(node, *, node->left_operand, node->right_operand, type); break;
                case '-': BINARY_OP(node, -, node->left_operand, node->right_operand, type); break;
                case '/': BINARY_OP(node, /, node->left_operand, node->right_operand, type); break;
                default:
                    fprintf(stderr, "Error, unknown operator `%c`\n", node->token.op.operator);
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
    // TODO: implement `ast_clean(Ast *ast)`
}

Token token_next(Token_List *tl, size_t *tkc)
{
    if (*tkc >= tl->count)
        return (Token) { .type = TYPE_NONE };
    else {
        Token tk = tl->tokens[*tkc];
        *tkc += 1;
        return tk;
    }
}

Token_Type token_peek(Token_List *tl, size_t *tkc)
{
    if (*tkc >= tl->count)
        return TYPE_NONE;
    else {
        Token_Type type = tl->tokens[*tkc].type;
        return type;
    }
}

Operator_Type peek_op(Token tk)
{
    if (tk.type != TYPE_OPERATOR) return OP_NONE;
    else return tk.op.type;
}

void print_token(Token tk)
{
    switch (tk.type) {
        case TYPE_VALUE: {
            if (tk.val.type == FLOAT) 
                printf("float: `%lf`\n", tk.val.f64);
            else 
                printf("int: `%ld`\n", tk.val.i64);
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
            exit(1);
    }
}

void print_tl(Token_List *tl)
{
    for (size_t i = 0; i < tl->count; ++i) {
        print_token(tl->tokens[i]);
    }
}

Ast_Node *ast_node_create(Token tk)
{
    Ast_Node *node = malloc(sizeof(Ast_Node));
    node->token = tk;
    node->left_operand = NULL;
    node->right_operand = NULL;
    return node;
}

Ast_Node *parse_terminal(Token tk, Token_List *tl, size_t *tkc)
{
    Ast_Node *val1 = ast_node_create(tk);
    Token_Type tk_type = token_peek(tl, tkc);

    if (tk_type == TYPE_OPERATOR) {
        Token opr = token_next(tl, tkc);
        Operator_Type op_type = peek_op(opr);

        if (op_type == OP_MULT || op_type == OP_DIV) {
            Ast_Node *op = ast_node_create(opr);
            Token t = token_next(tl, tkc);

            if (t.type == TYPE_NONE) {
                fprintf(stderr, "Error: expected second operand\n");
                exit(1);
            }

            Ast_Node *val2 = ast_node_create(t);  

            op->left_operand = val1;
            op->right_operand = val2;

            return op;
        } else {
            *tkc -= 1;
        }
    }

    return val1;
}

// TODO: increment count of ast
void ast_push_subtree(Ast *ast, Ast_Node *subtree)
{
    if (ast->root == NULL) {
        ast->root = subtree;
    } else {
        if (subtree->right_operand != NULL) {
            subtree->left_operand = ast->root;
            ast->root = subtree;
        } else {
            subtree->right_operand = ast->root;
            ast->root = subtree;
        }
    }
}

void parse_tokens(Ast *ast, Token_List *tl)
{
    size_t tkc = 0; // token counter
    while (1) {
        Token tk = token_next(tl, &tkc);
        if (tk.type == TYPE_NONE) break;
        
        if (tk.type == TYPE_VALUE) {
            Ast_Node *val1;
            Ast_Node *opr;
            Ast_Node *val2;

            val1 = parse_terminal(tk, tl, &tkc);

            Token_Type type = token_peek(tl, &tkc);
            if (type == TYPE_OPERATOR) {
                Token op = token_next(tl, &tkc);
                if (op.op.type == OP_MINUS || op.op.type == OP_PLUS) {
                    opr = ast_node_create(op);

                    Token v2 = token_next(tl, &tkc);
                    
                    if (v2.type != TYPE_VALUE) {
                        fprintf(stderr, "Error: expected second operator\n");
                        exit(1);
                    } 
                    
                    val2 = parse_terminal(v2, tl, &tkc);

                    opr->left_operand = val1;
                    opr->right_operand = val2;

                    ast_push_subtree(ast, opr);
                }
            } else {
                fprintf(stderr, "Error: not enough operands\n");
                exit(1);
            }
        } else if (tk.type == TYPE_OPERATOR) {
            Ast_Node *opr = ast_node_create(tk);
            Token val = token_next(tl, &tkc);
            
            if (val.type == TYPE_NONE || val.type != TYPE_VALUE) {
                fprintf(stderr, "Error: not enough operands\n");
                exit(1);
            }

            opr->right_operand = parse_terminal(val, tl, &tkc);
            ast_push_subtree(ast, opr);
        }
    }
}

int main(void)
{
    Ast ast = {0};
    Token_List tl;

    tl = tokensizer(sv_from_cstr("1 * 45 - 2 * 1 + 2 * 10 + 3 * 8 + 32 / 2"));

    print_tl(&tl);
    parse_tokens(&ast, &tl);
    print_tree(ast.root);

    resolve_ast(&ast);
    print_node(ast.root);
    
    tl_clean(&tl);
    return 0;
}