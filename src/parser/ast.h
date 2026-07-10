#ifndef AST_H
#define AST_H

typedef enum
{
    AST_INTEGER,
    AST_PRINT
} ASTNodeType;

typedef struct ASTNode
{
    ASTNodeType type;
    union
    {
        int intValue;
        struct ASTNode *expr;
    } data;
} ASTNode;

ASTNode *make_integer(int value);
ASTNode *make_print(ASTNode *expr);
void free_ast(ASTNode *node);

#endif