#ifndef AST_H
#define AST_H

typedef enum
{
    AST_INTEGER,
    AST_FLOAT,
    AST_PRINT,
    AST_STRING,
    AST_CHAR,
    AST_BOOL,
    AST_PROGRAM
} ASTNodeType;

typedef struct ASTNode
{
    ASTNodeType type;
    union
    {
        int intValue;
        double floatValue;
        char *strValue;
        char charValue;
        int boolValue;
        struct ASTNode *expr; // for AST_PRINT
        struct
        {
            struct ASTNode **statements; // array of statement ASTNode*
            int count;
            int capacity;
        } program; // for AST_PROGRAM
    } data;
} ASTNode;

ASTNode *make_integer(int value);
ASTNode *make_print(ASTNode *expr);
ASTNode *make_string(const char *value);
ASTNode *make_program(void);
ASTNode *make_float(double value);
ASTNode *make_char(char value);
ASTNode *make_bool(int value);

void program_add_statement(ASTNode *program, ASTNode *stmt); // adds a child to the program
void free_ast(ASTNode *node);

#endif