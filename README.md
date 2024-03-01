# Simple AST
***

## About project

This tiny project implemented for my [virtual machine](https://github.com/ambr0siaa/LunaVM) to parse arefmetic expresions in assembly instructions args like this:
```nasm
add r1, 2 * (2 + 1) + 27
```

## Eval

To implement eval I use simple [lexer](./lexer.h) which can analyze numbers, operators and variables and parser which generate AST. Eval take as input AST and return node of AST which contain final answer.

## Note

* To use float numbers, write with `.0`, and furthermore, all numbers in the expression must be with `.0` if float, if int, write nothing at all.

* Parser can analyze only `+ - * /`

* Usage for using variables
    ```c
    Var_List vl = {0}; // Create varibale list

    // first arg is variable name which parser can read from expresion
    // second arg is value, use macro for int -> VALUE_INT(); for float -> VALUE_FLOAT() 
    Variable var = var_create("var", VALUE_INT(3)); 

    // Push variable into varibale list
    var_push(&vl, var);
    ```