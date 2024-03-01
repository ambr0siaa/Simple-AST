#include "../include/parser.h"

int main(void)
{
    Var_List vl = {0};
    Variable arsenii = var_create("arsenii", VALUE_INT(3));
    var_push(&vl, arsenii);

    char *tests[] = { 
        "23 * (32 * (arsenii*(4 * (45 + arsenii* (1234 - 3434 * (arsenii - 1))) + 3 * (234 + 5) + 2 * (32 + 4)) + 56) + 4 * (45 + 1) + arsenii * (234 + 5) + 2 * (32 + 4))",
        "4 * (((32 + 4 + 1) * (5 + 3 + 6) * (2 + 3 + 5 + 6)) + ((9 * (2 + 3 - 4) * 3 + (5 + 6 - 3)) * 2) * ((3 * 3 * 3) - (4 - 5) * 3 * 3))",
        "((2 * (1 + 3 + 4 - 6) * (23 - 2) * 3 + (34 + 4) * (6 * 7 - 2 * (3 + 1 - 2) * 2 * 4) * (3 * (3 -  45) + 5 * (34 + 45) * (434 - 2)) + (3 - 5)) + ( 3 * ( 5 * ( 4 * (34 + 6) - 54) - 45) * 4))",
        "98721354+2355467*1654567+23445467*(2345467-2384567)+38676*8534567-3453456+(3454565*54675345)*(3400-645)"
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        Ast ast = {0};

        printf("\n\n--------------------------- Test%zu ---------------------------\n\n", i);
        Lexer lex = lexer(sv_from_cstr(tests[i]), &vl);
        print_lex(&lex);

        parser(&ast, &lex);
        print_ast(&ast);
        
        eval(&ast);

        printf("Answer:\n\t");
        print_node(ast.root);
        printf("\n\n--------------------------- Test%zuEnd ------------------------------\n\n",i);
        
        ast_clean(ast.root, &ast.count);
        lex_clean(&lex);
    }
    
    var_clean(&vl);
    return 0;
}