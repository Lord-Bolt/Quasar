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

// for let and variable
ASTNode *make_let(const char *name, VarType type, ASTNode *init)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_LET;
    node->data.let.name = strdup(name);
    node->data.let.vartype = type;
    node->data.let.init = init;
    return node;
}

ASTNode *make_variable(const char *name)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_VARIABLE;
    node->data.varName = strdup(name);
    return node;
}

// for reassignment of var
ASTNode *make_assign(const char *name, ASTNode *value)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_ASSIGN;
    node->data.assign.name = strdup(name);
    node->data.assign.value = value;
    return node;
}

// for binary ops
ASTNode *make_binary(BinaryOp op, ASTNode *left, ASTNode *right)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_BINARY;
    node->data.binary.op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;
    return node;
}

// for {...}
ASTNode *make_block(void)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_BLOCK;
    node->data.block.statements = malloc(sizeof(ASTNode *) * 4);
    if (!node->data.block.statements)
    {
        free(node);
        return NULL;
    }
    node->data.block.capacity = 4;
    node->data.block.count = 0;
    return node;
}

void block_add_statement(ASTNode *block, ASTNode *stmt)
{
    if (block->data.block.count >= block->data.block.capacity)
    {
        block->data.block.capacity *= 2;
        ASTNode **newbuf = realloc(block->data.block.statements,
                                   sizeof(ASTNode *) * block->data.block.capacity);
        if (!newbuf)
            return; // memory error – we can improve later
        block->data.block.statements = newbuf;
    }
    block->data.block.statements[block->data.block.count++] = stmt;
}

// for unary ops - !, &, |
ASTNode *make_unary(UnaryOp op, ASTNode *operand)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_UNARY;
    node->data.unary.op = op;
    node->data.unary.operand = operand;
    return node;
}

// for if...elif...else
ASTNode *make_if(ASTNode *condition, ASTNode *body, ASTNode *next)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_IF;
    node->data.ifelse.condition = condition;
    node->data.ifelse.body = body;
    node->data.ifelse.next = next;
    return node;
}

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
    case AST_LET:
        free(node->data.let.name);
        free_ast(node->data.let.init);
        break;
    case AST_VARIABLE:
        free(node->data.varName);
        break;
    case AST_ASSIGN:
        free(node->data.assign.name);
        free_ast(node->data.assign.value);
        break;
    case AST_BINARY:
        free_ast(node->data.binary.left);
        free_ast(node->data.binary.right);
        break;
    case AST_BLOCK:
        for (int i = 0; i < node->data.block.count; i++)
            free_ast(node->data.block.statements[i]);
        free(node->data.block.statements);
        break;
    case AST_UNARY:
        free_ast(node->data.unary.operand);
        break;
    case AST_IF:
        free_ast(node->data.ifelse.condition);
        free_ast(node->data.ifelse.body);
        free_ast(node->data.ifelse.next);
        break;
    default:
        break;
    }
    free(node);
}