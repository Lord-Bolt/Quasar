#include <stdio.h>
#include "parser/parser.h"

int main(int argc, char **argv)
{
    const char *source = "print(42);";
    ASTNode *ast = parse_program(source);
    if (ast)
    {
        if (ast->type == AST_PRINT && ast->data.expr->type == AST_INTEGER)
        {
            printf("Success: print statement with integer %d\n", ast->data.expr->data.intValue);
        }
        free_ast(ast);
    }
    return 0;
}