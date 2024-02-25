#include "./parser.h"

int main(void)
{
    Ast ast = {0};
    Lexer lex = {0};
    Var_List vl = {0};

    Variable arsenii = var_create("arsenii", VALUE_INT(3));
    var_push(&vl, arsenii);

    char *test = "23 * (32 * (arsenii*(4 * (45 + arsenii* (1234 - 3434 * (arsenii - 1))) + 3 * (234 + 5) + 2 * (32 + 4)) + 56) + 4 * (45 + 1) + arsenii * (234 + 5) + 2 * (32 + 4))";
    char *test2 = "4 * (((32 + 4 + 1) * (5 + 3 + 6) * (2 + 3 + 5 + 6)) + ((9 * (2 + 3 - 4) * 3 + (5 + 6 - 3)) * 2) * ((3 * 3 * 3) - (4 - 5) * 3 * 3))";

    lex = lexer(sv_from_cstr(test2), &vl);
    print_lex(&lex);

    parser(&ast, &lex);
    print_ast(&ast);
    
    eval(&ast);

    printf("Answer:\n\t");
    print_node(ast.root);
    
    ast_clean(ast.root, &ast.count);
    lex_clean(&lex);
    return 0;
}