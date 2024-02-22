#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define SV_IMPLEMENTAION
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
    size_t tp;         // Token Pointer
} Lexer;

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

#define NONE_MODE 0
#define FLOAT_MODE 1
#define INT_MODE 2

Value parse_value(String_View sv, int mode)
{
    int is_float;

    if (mode == NONE_MODE) is_float = sv_is_float(sv);
    else is_float = mode;

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

#define LEX_INIT_CAPACITY 128

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

            if (sv_is_float(value)) tk.val = parse_value(value, FLOAT_MODE);
            else tk.val = parse_value(value, INT_MODE);
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
                        exit(1);
                }
                tk.op.operator = opr.data[0];
            }
        }
        lex_push(&lex, tk);
    }

    return lex;
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

void print_ast(Ast_Node *node)
{
    static int i;

    TAB(i);
    print_node(node);

    if (node->right_operand != NULL) {
        TAB(i);
        printf("right: ");
        print_ast(node->right_operand);
        i--;
    }

    if (node->left_operand != NULL) {
        TAB(i);
        printf("left: ");
        print_ast(node->left_operand);
        i--;
    }

    i--;
}

#define BINARY_OP(dst, operator, op1, op2, type)                                        \
    do {                                                                                \
        if (type == 'f')                                                                \
            (dst)->token.val.f64 = (op1)->token.val.f64 operator (op2)->token.val.f64;  \
        else if (type == 'i')                                                           \
            (dst)->token.val.i64 = (op1)->token.val.i64 operator (op2)->token.val.i64;  \
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

void ast_clean(Ast_Node *node)
{
    if (node->token.type == TYPE_OPERATOR) {
        if (node->left_operand->token.type == TYPE_VALUE && 
            node->right_operand->token.type == TYPE_VALUE) {
            free(node->right_operand);
            free(node->left_operand);
        }
    } else if (node->token.type == TYPE_VALUE) { 
        return;
    } else {
        ast_clean(node->left_operand);
        ast_clean(node->right_operand);
    }
}

Token token_next(Lexer *lex)
{
    if (lex->tp >= lex->count) return (Token) { .type = TYPE_NONE };
    else {
        Token tk = lex->tokens[lex->tp];
        lex->tp += 1;
        return tk;
    }
}

Token_Type token_peek(Lexer *lex)
{
    if (lex->tp >= lex->count) return TYPE_NONE;
    else {
        Token_Type type = lex->tokens[lex->tp].type;
        return type;
    }
}

Operator_Type operator_peek(Token tk)
{
    if (tk.type != TYPE_OPERATOR) return OP_NONE;
    else return tk.op.type;
}

void print_token(Token tk)
{
    switch (tk.type) {
        case TYPE_VALUE: {
            if (tk.val.type == FLOAT) printf("float: `%lf`\n", tk.val.f64);
            else printf("int: `%ld`\n", tk.val.i64);
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

void print_lex(Lexer *lex)
{
    for (size_t i = 0; i < lex->count; ++i) {
        print_token(lex->tokens[i]);
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

// E: T { + | -  T }*
// T: V { * | /  V }*
// V: INT | FLOAT

// TODO: support many *
// TODO: function parse_expr
Ast_Node *parse_term(Token tk, Lexer *lex)
{
    Ast_Node *val1 = ast_node_create(tk);
    Ast subtree = {0};

    do {
        Token_Type tk_type = token_peek(lex);
        if (tk_type == TYPE_NONE) break;

        if (tk_type == TYPE_OPERATOR) {
            Token opr_tk = token_next(lex);
            Operator_Type op_type = operator_peek(opr_tk);

            if (op_type == OP_MULT || op_type == OP_DIV) {
                Ast_Node *opr_node = ast_node_create(opr_tk);
                Token t = token_next(lex);

                if (t.type == TYPE_NONE) {
                    fprintf(stderr, "Error: expected second operand\n");
                    exit(1);
                }

                Ast_Node *val2 = ast_node_create(t);  

                opr_node->left_operand = val1;
                opr_node->right_operand = val2;

                ast_push_subtree(&subtree, opr_node);
            } else {
                lex->tp -= 1;
                break;
            }
        } else break;
    } while (1);

    if (subtree.root == NULL) return val1;
    else return subtree.root;
}

// TODO: parsing brackets, many * and other things
void parser(Ast *ast, Lexer *lex)
{
    while (1) {
        Token tk = token_next(lex);
        if (tk.type == TYPE_NONE) break;
        
        if (tk.type == TYPE_VALUE) {
            Ast_Node *val1;
            Ast_Node *opr;
            Ast_Node *val2;

            val1 = parse_term(tk, lex);

            Token_Type type = token_peek(lex);
            if (type == TYPE_OPERATOR) {
                Token op = token_next(lex);
                if (op.op.type == OP_MINUS || op.op.type == OP_PLUS) {
                    opr = ast_node_create(op);

                    Token v2 = token_next(lex);
                    
                    if (v2.type != TYPE_VALUE) {
                        fprintf(stderr, "Error: expected second operator\n");
                        exit(1);
                    } 
                    
                    val2 = parse_term(v2, lex);

                    opr->left_operand = val1;
                    opr->right_operand = val2;

                    ast_push_subtree(ast, opr);
                } else if (op.op.type == OP_MULT || op.op.type == OP_DIV) {
                    opr = ast_node_create(op);
                    Token t = token_next(lex);
                    val2 = parse_term(t, lex);

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
            Token val = token_next(lex);
            
            if (val.type == TYPE_NONE || val.type != TYPE_VALUE) {
                fprintf(stderr, "Error: not enough operands\n");
                exit(1);
            }

            opr->right_operand = parse_term(val, lex);
            ast_push_subtree(ast, opr);
        }
    }
}

void ast_delete(Ast *ast)
{
    ast_clean(ast->root);
    if (ast->count == 1) 
        free(ast->root);
}

int main(void)
{
    Ast ast = {0};
    Lexer lex;

    char *test1 = "1 * 45 * 2 * 3 * 1 * 1 - 2 * 1 * 2 * 3 * 10 + 2 * 10 * 3 + 3 * 8 * 3 + 32 / 2 + 1 * 1";
    char *test2 = "2 * 2 * 2 + 1";

    lex = lexer(sv_from_cstr(test1));
    print_lex(&lex);

    parser(&ast, &lex);

    printf("\n\n");
    print_ast(ast.root);

    resolve_ast(&ast);
    print_node(ast.root);
    
    ast_delete(&ast);
    lex_clean(&lex);
    return 0;
}