#include "./parser.h"

int main(void)
{
    Ast ast = {0};
    Lexer lex;

    char *test = "23 * (32 * (3 * (4 * (45 + 3 * (1234 - 3434 * (3 - 1))) + 3 * (234 + 5) + 2 * (32 + 4)) + 56) + 4 * (45 + 1) + 3 * (234 + 5) + 2 * (32 + 4))";

    lex = lexer(sv_from_cstr(test));
    print_lex(&lex);

    parser(&ast, &lex);
    print_ast(&ast);
    
    resolve_ast(&ast);

    printf("Answer:\n\t");
    print_node(ast.root);
    
    ast_clean(ast.root, &ast.count);
    lex_clean(&lex);
    return 0;
}