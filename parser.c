#include "./parser.h"

void print_node(Ast_Node *node)
{
    switch (node->token.type) {
        case TYPE_VALUE:
            if (node->token.val.type == VAL_FLOAT) {
                printf("value: '%lf'\n", node->token.val.f64);
            } else {
                printf("value: '%ld'\n", node->token.val.i64);
            }
            break;
        case TYPE_OPERATOR:
            printf("opr: '%c'\n", node->token.op.operator);
            break;
        default:
            break;
    }
}

void print_ast_root(Ast_Node *node)
{
    static int i;

    TAB(i);
    print_node(node);

    if (node->right_operand != NULL) {
        TAB(i);
        printf("right: ");
        print_ast_root(node->right_operand);
        i--;
    }

    if (node->left_operand != NULL) {
        TAB(i);
        printf("left: ");
        print_ast_root(node->left_operand);
        i--;
    }

    i--;
}          

void print_ast(Ast *ast)
{
    printf("\n------------------- Abstract Syntax Tree -------------------\n\n");
    print_ast_root(ast->root);
    printf("\n------------------------------------------------------------\n\n");
}

Ast_Node *resolve_ast(Ast_Node *node)
{   
    if (node->left_operand != NULL && node->right_operand != NULL) {
        if (node->right_operand->token.type == TYPE_OPERATOR ||
            node->left_operand->token.type == TYPE_OPERATOR ) {
                node->left_operand = resolve_ast(node->left_operand);
                node->right_operand = resolve_ast(node->right_operand);
        }
            
        if (node->token.type == TYPE_OPERATOR) {
            char type;
            if (node->left_operand->token.val.type == VAL_FLOAT) {
                type = 'f'; 
                node->token.val.type = VAL_FLOAT;
            } else {
                type = 'i'; 
                node->token.val.type = VAL_INT;
            } 
            
            node->token.type = TYPE_VALUE;

            switch (node->token.op.operator) {
                case '+': BINARY_OP(node, +, node->left_operand, node->right_operand, type); break;
                case '*': BINARY_OP(node, *, node->left_operand, node->right_operand, type); break;
                case '-': BINARY_OP(node, -, node->left_operand, node->right_operand, type); break;
                case '/': BINARY_OP(node, /, node->left_operand, node->right_operand, type); break;
                default: {
                    fprintf(stderr, "Error, unknown operator `%c`\n", node->token.op.operator);
                    EXIT;
                }
            }

            free(node->left_operand);
            free(node->right_operand);
            return node;
        }
    }
    return node;
}

// Get ast and calculate final number
void eval(Ast *ast)
{
    ast->root = resolve_ast(ast->root);
    ast->count = 1; 
}

void ast_clean(Ast_Node *node, size_t *node_count)
{
    if (*node_count == 1) free(node);
    else {
        if (node->token.type == TYPE_OPERATOR) {
            if (node->left_operand->token.type == TYPE_VALUE && 
                node->right_operand->token.type == TYPE_VALUE) {
                free(node->right_operand);
                free(node->left_operand);
                *node_count -= 2; 
            }
        } else if (node->token.type == TYPE_VALUE) { 
            return;
        } else {
            ast_clean(node->left_operand, node_count);
            ast_clean(node->right_operand, node_count);
        }
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

/*
*  Grammar:
*
*   E - expresion
*   T - term
*   V - value
*   
*   * - mean that can be one or more T, V or E
*   
*   E: T { + | -  T }*
*   T: V { * | /  V }*
*   V: INT | FLOAT
*/

Ast_Node *parse_expr(Token tk, Lexer *lex)
{
    if (tk.type == TYPE_VALUE) {
        Ast_Node *val1;
        Ast_Node *opr;
        Ast_Node *val2;

        val1 = parse_term(tk, lex);
        Token_Type type;

        do {
            type = token_peek(lex);
            if (type == TYPE_OPERATOR) {
                Token op = token_next(lex);
                if (op.op.type == OP_MINUS || op.op.type == OP_PLUS) {
                    opr = ast_node_create(op);

                    Token v2 = token_next(lex);
                    
                    if (v2.type == TYPE_OPEN_BRACKET) {
                        Token t1 = token_next(lex);
                        val2 = parse_expr(t1, lex);

                    } else if (v2.type == TYPE_VALUE) {
                        val2 = parse_term(v2, lex);

                    } else {
                        fprintf(stderr, "Error: in function `parse_expr` unknown condition\n");
                        EXIT;
                    }

                    if (val2 == NULL) EXIT;
                    
                    opr->left_operand = val1;
                    opr->right_operand = val2;
                    val1 = opr;
                }
            } else if (type == TYPE_OPEN_BRACKET) {
                Token t1 = token_next(lex);
                Ast_Node *subtree = parse_expr(t1, lex);

                if (subtree == NULL) EXIT;
                return subtree;

            } else if (type == TYPE_NONE) {
                return val1;

            } else if (type == TYPE_CLOSE_BRACKET) {
                token_next(lex);
                return val1;

            } else if (type == TYPE_VALUE) {
                fprintf(stderr, "TODO: `TYPE_VALUE` not implemented\n");
                EXIT;

            } else {
                fprintf(stderr, "Error: in `parse_expr` unknown token type `%u`\n", type);
                EXIT;
            }
        } while(1); 

        return val1;

    } else if (tk.type == TYPE_OPEN_BRACKET) {
        Token t1 = token_next(lex);
        Ast_Node *subtree = parse_expr(t1, lex);

        // TODO: rework this part to parse more brackets
        Token t2 = token_next(lex);
        if (t2.type == TYPE_OPERATOR) {
            Ast_Node *val;
            Ast_Node *opr_node = ast_node_create(t2);

            Token t3 = token_next(lex);
            if (t3.type == TYPE_OPEN_BRACKET) {
                t3 = token_next(lex);
                val = parse_expr(t3, lex);

            } else if (t3.type == TYPE_VALUE) {
                val = parse_term(t3, lex);

            } else {
                fprintf(stderr, "unknown type `%u`\n", t3.type);
                EXIT;
            }

            if (val == NULL) EXIT;

            opr_node->right_operand = val;
            opr_node->left_operand = subtree;
            
            return opr_node;

        } else {
            lex->tp -= 1;
            return subtree;
        }
    } else {
        fprintf(stderr, "Error: unknown type in function `parse_expr` in 1st condition\n");
        return NULL;
    }
}

void subtree_node_count(Ast_Node *subtree, size_t *count) 
{
    if (subtree == NULL) return;
    subtree_node_count(subtree->left_operand, count);
    *count += 1;
    subtree_node_count(subtree->right_operand, count);
}

Ast_Node *parse_term(Token tk, Lexer *lex)
{   
    Ast_Node *val1 = ast_node_create(tk);  
    Ast subtree = {0};

    do {
        Token_Type tk_type = token_peek(lex);
        if (tk_type == TYPE_NONE) break;

        if (tk_type == TYPE_OPERATOR) {
            Ast_Node *val2;
            Token opr_tk = token_next(lex);

            if (opr_tk.op.type == OP_MULT || opr_tk.op.type == OP_DIV) {
                Ast_Node *opr_node = ast_node_create(opr_tk);
                Token t1 = token_next(lex);

                if (t1.type == TYPE_VALUE) {
                    val2 = ast_node_create(t1);

                } else if (t1.type == TYPE_OPEN_BRACKET) {
                    Token tok = token_next(lex); 
                    val2 = parse_expr(tok, lex);
            
                } else if (t1.type == TYPE_NONE) {
                    fprintf(stderr, "Error: expected second operand\n");
                    EXIT;
                }

                if (val2 == NULL) EXIT;

                opr_node->left_operand = val1;
                opr_node->right_operand = val2;
                ast_push_subtree(&subtree, opr_node);

            } else {
                lex->tp -= 1;
                break;
            }
        } else {
            break;
        }
    } while (1);

    if (subtree.root == NULL) return val1;
    else return subtree.root;
}

void parser(Ast *ast, Lexer *lex)
{
    while (1) {
        Token tk = token_next(lex);
        if (tk.type == TYPE_NONE) break;
        
        size_t count = 0;
        if (tk.type == TYPE_VALUE) {
            Token_Type type = token_peek(lex);
            if (type == TYPE_OPERATOR) {
                Token t1 = token_next(lex);
                if (t1.op.type == OP_MINUS || t1.op.type == OP_PLUS) {
                    lex->tp -= 1;

                    Ast_Node *subtree = parse_expr(tk, lex);
                    if (subtree == NULL) EXIT;

                    subtree_node_count(subtree, &count);
                    ast_push_subtree(ast, subtree);
                } else if (t1.op.type == OP_MULT || t1.op.type == OP_DIV) {
                    lex->tp -= 1;
                    Ast_Node *val = parse_term(tk, lex);
                    subtree_node_count(val, &count);
                    ast_push_subtree(ast, val);
                }
            }

        } else if (tk.type == TYPE_OPERATOR) {
            Ast_Node *val;
            Ast_Node *opr = ast_node_create(tk);
            Token tok = token_next(lex);

            if (tok.type == TYPE_VALUE) {
                val = parse_term(tok, lex);
            
            } else if (tok.type == TYPE_OPEN_BRACKET) {
                Token t = token_next(lex);
                val = parse_expr(t, lex);    

            } else {
                fprintf(stderr, "Error: in function `parser` unknown condition\n");
                EXIT;
            }

            if (val == NULL) EXIT;

            opr->right_operand = val;
            subtree_node_count(val, &count);
            ast_push_subtree(ast, opr);

        } else if (tk.type == TYPE_OPEN_BRACKET) {
            Token tok = token_next(lex);
            Ast_Node *subtree = parse_expr(tok, lex);
            if (subtree == NULL) EXIT;

            subtree_node_count(subtree, &count);
            ast_push_subtree(ast, subtree);

        } else if (tk.type == TYPE_CLOSE_BRACKET) {
            continue;
        }

        ast->count += count;
    }
}