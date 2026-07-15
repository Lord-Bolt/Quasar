#ifndef AST_H
#define AST_H
#include "symtab/symtab.h"

typedef enum
{
    AST_INTEGER,
    AST_FLOAT,
    AST_PRINT,
    AST_STRING,
    AST_CHAR,
    AST_BOOL,
    AST_LET,
    AST_VARIABLE,
    AST_ASSIGN,
    AST_PROGRAM,
    AST_BINARY,
    AST_BLOCK
} ASTNodeType;

typedef enum
{
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_POW,
    OP_FLDIV,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE
} BinaryOp;

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
        struct
        {
            struct ASTNode **expressions; // array of expression nodes
            int count;
            int capacity;
        } print; // for AST_PRINT

        struct
        {
            char *name;
            VarType vartype;
            struct ASTNode *init; // initializer expression
        } let;                    // for AST_LET
        char *varName;            // for AST_VARIABLE

        struct
        {
            struct ASTNode **statements; // for AST_PROGRAM
            int count;
            int capacity;
        } program;

        struct
        {
            char *name;            // variable name
            struct ASTNode *value; // right-hand expression
        } assign;                  // for AST_ASSIGN

        struct
        {
            BinaryOp op;
            struct ASTNode *left; // for binary operations
            struct ASTNode *right;
        } binary;

        struct
        {
            struct ASTNode **statements; // array of statement nodes
            int count;
            int capacity;
        } block; // for AST_BLOCK
    } data;
} ASTNode;

ASTNode *make_integer(int value);
ASTNode *make_print_empty(void);
ASTNode *make_string(const char *value);
ASTNode *make_program(void);
ASTNode *make_float(double value);
ASTNode *make_char(char value);
ASTNode *make_bool(int value);
ASTNode *make_let(const char *name, VarType type, ASTNode *init);
ASTNode *make_variable(const char *name);
ASTNode *make_assign(const char *name, ASTNode *value);
ASTNode *make_binary(BinaryOp op, ASTNode *left, ASTNode *right);
ASTNode *make_block(void);

void block_add_statement(ASTNode *block, ASTNode *stmt);
void print_add_expression(ASTNode *print_node, ASTNode *expr); // add an expression to the print node
void program_add_statement(ASTNode *program, ASTNode *stmt);   // adds a child to the program
void free_ast(ASTNode *node);

#endif