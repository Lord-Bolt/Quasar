#include "ast.h"
#include <stdlib.h>

ASTNode *make_integer(int value)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_INTEGER;
    node->data.intValue = value;
    return node;
}

ASTNode *make_print(ASTNode *expr)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_PRINT;
    node->data.expr = expr;
    return node;
}

void free_ast(ASTNode *node)
{
    if (!node)
        return;
    if (node->type == AST_PRINT)
    {
        free_ast(node->data.expr);
    }
    // for other types, free children here
    free(node);
}