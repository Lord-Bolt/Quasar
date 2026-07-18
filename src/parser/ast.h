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
    AST_MULTI_LET,
    AST_VARIABLE,
    AST_ASSIGN,
    AST_PROGRAM,
    AST_BINARY,
    AST_UNARY,
    AST_BLOCK,
    AST_IF,
    AST_WHILE,
    AST_REPEAT_UNTIL,
    AST_FOR,
    AST_EXPR_STATEMENT,
    AST_BREAK,
    AST_CONTINUE,
    AST_MATCH,
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
    OP_GE,
    OP_AND,
    OP_OR,
    OP_ASSIGN,
    OP_ADD_ASSIGN,
    OP_SUB_ASSIGN,
    OP_MUL_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_POW_ASSIGN,
    OP_FLDIV_ASSIGN
} BinaryOp;

typedef enum
{
    UNARY_NOT,      // !
    UNARY_MINUS,    // -x
    UNARY_PLUS,     // +x
    UNARY_PRE_INC,  // ++x
    UNARY_PRE_DEC,  // --x
    UNARY_POST_INC, // x++
    UNARY_POST_DEC  // x--
} UnaryOp;

typedef struct
{
    struct ASTNode *value; // NULL for default
    struct ASTNode *body;  // block of statements
} MatchCase;

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

        struct
        {
            UnaryOp op;
            struct ASTNode *operand; // for unary operations
        } unary;

        struct
        {
            struct ASTNode *condition; // the if-condition expression
            struct ASTNode *body;      // block to execute if true
            struct ASTNode *next;      // linked list of elif/else parts (could be another AST_IF)
        } ifelse;

        struct
        {
            struct ASTNode *condition;
            struct ASTNode *body;
        } whileloop; // for AST_WHILE
        struct
        {
            struct ASTNode *condition;
            struct ASTNode *body;
        } repeatuntil; // for AST_REPEAT_UNTIL

        struct
        {
            struct ASTNode *init;      // optional (let or assignment statement)
            struct ASTNode *condition; // optional expression
            struct ASTNode *update;    // optional expression
            struct ASTNode *body;      // block
        } forloop;

        struct ASTNode *expr; // for AST_EXPR_STATEMENT

        struct
        {
            struct ASTNode *discriminant; // <-- was ASTNode *, now struct ASTNode *
            MatchCase *cases;
            int case_count;
            int case_capacity;
        } match;

        struct
        {
            struct ASTNode **declarations; // array of AST_LET nodes
            int count;
            int capacity;
        } multilet;
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
ASTNode *make_unary(UnaryOp op, ASTNode *operand);
ASTNode *make_if(ASTNode *condition, ASTNode *body, ASTNode *next);
ASTNode *make_while(ASTNode *condition, ASTNode *body);
ASTNode *make_repeat_until(ASTNode *condition, ASTNode *body);
ASTNode *make_for(ASTNode *init, ASTNode *condition, ASTNode *update, ASTNode *body);
ASTNode *make_expr_statement(ASTNode *expr);
ASTNode *make_break(void);
ASTNode *make_continue(void);
ASTNode *make_match(ASTNode *discriminant);
ASTNode *make_multilet(void);

void multilet_add(ASTNode *multilet, ASTNode *decl);
void match_add_case(ASTNode *match, ASTNode *value, ASTNode *body);
void block_add_statement(ASTNode *block, ASTNode *stmt);       // allows {...} to be used
void print_add_expression(ASTNode *print_node, ASTNode *expr); // add an expression to the print node
void program_add_statement(ASTNode *program, ASTNode *stmt);   // adds a child to the program
void free_ast(ASTNode *node);

#endif