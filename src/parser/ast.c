#define _POSIX_C_SOURCE 200809L
#include "ast.h"
#include <stdlib.h>
#include <string.h>

// For int
ASTNode *make_integer(int value)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_INTEGER;
    node->data.intValue = value;
    return node;
}

// For Float
ASTNode *make_float(double value)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_FLOAT;
    node->data.floatValue = value;
    return node;
}

// For print stmt
ASTNode *make_print_empty(void)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_PRINT;
    node->data.print.count = 0;
    node->data.print.capacity = 4; // start small
    node->data.print.expressions = malloc(sizeof(ASTNode *) * node->data.print.capacity);
    if (!node->data.print.expressions)
    {
        free(node);
        return NULL;
    }
    return node;
}

void print_add_expression(ASTNode *print_node, ASTNode *expr)
{
    if (print_node->data.print.count >= print_node->data.print.capacity)
    {
        print_node->data.print.capacity *= 2;
        ASTNode **newbuf = realloc(print_node->data.print.expressions,
                                   sizeof(ASTNode *) * print_node->data.print.capacity);
        if (!newbuf)
            return; // memory error, but we'll ignore for now
        print_node->data.print.expressions = newbuf;
    }
    print_node->data.print.expressions[print_node->data.print.count++] = expr;
}

// For string
ASTNode *make_string(const char *value)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_STRING;
    node->data.strValue = strdup(value);
    return node;
}

// For single chars
ASTNode *make_char(char value)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_CHAR;
    node->data.charValue = value;
    return node;
}

// For bool (t/f)
ASTNode *make_bool(int value)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_BOOL;
    node->data.boolValue = value;
    return node;
}

// For .qs
ASTNode *make_program(void)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_PROGRAM;
    node->data.program.capacity = 8;
    node->data.program.count = 0;
    node->data.program.statements = malloc(sizeof(ASTNode *) * node->data.program.capacity);
    if (!node->data.program.statements)
    {
        free(node);
        return NULL;
    }
    return node;
}

void program_add_statement(ASTNode *program, ASTNode *stmt)
{
    if (program->data.program.count >= program->data.program.capacity)
    {
        program->data.program.capacity *= 2;
        ASTNode **newbuf = realloc(program->data.program.statements,
                                   sizeof(ASTNode *) * program->data.program.capacity);
        if (!newbuf)
            return; // memory error; ignore for now
        program->data.program.statements = newbuf;
    }
    program->data.program.statements[program->data.program.count++] = stmt;
}

// Update free_ast to handle AST_PROGRAM
void free_ast(ASTNode *node)
{
    if (!node)
        return;
    switch (node->type)
    {
    case AST_PRINT:
        for (int i = 0; i < node->data.print.count; i++)
            free_ast(node->data.print.expressions[i]);
        free(node->data.print.expressions);
        break;
    case AST_STRING:
        free(node->data.strValue);
        break;
    case AST_PROGRAM:
        for (int i = 0; i < node->data.program.count; i++)
            free_ast(node->data.program.statements[i]);
        free(node->data.program.statements);
        break;
    case AST_INTEGER:
    case AST_FLOAT:
        // nothing to free
        break;
    default:
        break;
    }
    free(node);
}