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

ASTNode *make_multilet(void)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_MULTI_LET;
    node->data.multilet.capacity = 4;
    node->data.multilet.count = 0;
    node->data.multilet.declarations = (struct ASTNode **)malloc(
        sizeof(struct ASTNode *) * node->data.multilet.capacity);
    if (!node->data.multilet.declarations)
    {
        free(node);
        return NULL;
    }
    return node;
}

void multilet_add(ASTNode *multilet, ASTNode *decl)
{
    if (multilet->data.multilet.count >= multilet->data.multilet.capacity)
    {
        multilet->data.multilet.capacity *= 2;
        multilet->data.multilet.declarations = (struct ASTNode **)realloc(
            multilet->data.multilet.declarations,
            sizeof(struct ASTNode *) * multilet->data.multilet.capacity);
        if (!multilet->data.multilet.declarations)
            return; // memory error – ignore for now
    }
    multilet->data.multilet.declarations[multilet->data.multilet.count++] = decl;
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

// for while and repeat...until
ASTNode *make_while(ASTNode *condition, ASTNode *body)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_WHILE;
    node->data.whileloop.condition = condition;
    node->data.whileloop.body = body;
    return node;
}

ASTNode *make_repeat_until(ASTNode *condition, ASTNode *body)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_REPEAT_UNTIL;
    node->data.repeatuntil.condition = condition;
    node->data.repeatuntil.body = body;
    return node;
}

// for for loop
ASTNode *make_for(ASTNode *init, ASTNode *condition, ASTNode *update, ASTNode *body)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_FOR;
    node->data.forloop.init = init;
    node->data.forloop.condition = condition;
    node->data.forloop.update = update;
    node->data.forloop.body = body;
    return node;
}
ASTNode *make_expr_statement(ASTNode *expr)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_EXPR_STATEMENT;
    node->data.expr = expr;
    return node;
}

// for break and continue
ASTNode *make_break(void)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_BREAK;
    return node;
}

ASTNode *make_continue(void)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_CONTINUE;
    return node;
}

// for match...case...default
ASTNode *make_match(ASTNode *discriminant)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_MATCH;
    node->data.match.discriminant = discriminant;
    node->data.match.case_capacity = 4;
    node->data.match.cases = (MatchCase *)malloc(sizeof(MatchCase) * node->data.match.case_capacity);
    if (!node->data.match.cases)
    {
        free(node);
        return NULL;
    }
    node->data.match.case_count = 0;
    return node;
}

void match_add_case(ASTNode *match, ASTNode *value, ASTNode *body)
{
    if (match->data.match.case_count >= match->data.match.case_capacity)
    {
        match->data.match.case_capacity *= 2;
        match->data.match.cases = (MatchCase *)realloc(
            match->data.match.cases,
            sizeof(MatchCase) * match->data.match.case_capacity);
        if (!match->data.match.cases)
            return; // memory error – ignore for now
    }
    match->data.match.cases[match->data.match.case_count].value = value;
    match->data.match.cases[match->data.match.case_count].body = body;
    match->data.match.case_count++;
}

// for input
ASTNode *make_input(ASTNode *prompt)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node)
        return NULL;
    node->type = AST_INPUT;
    node->data.prompt = prompt;
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
    case AST_WHILE:
        free_ast(node->data.whileloop.condition);
        free_ast(node->data.whileloop.body);
        break;
    case AST_REPEAT_UNTIL:
        free_ast(node->data.repeatuntil.condition);
        free_ast(node->data.repeatuntil.body);
        break;
    case AST_FOR:
        if (node->data.forloop.init)
            free_ast(node->data.forloop.init);
        if (node->data.forloop.condition)
            free_ast(node->data.forloop.condition);
        if (node->data.forloop.update)
            free_ast(node->data.forloop.update);
        if (node->data.forloop.body)
            free_ast(node->data.forloop.body);
        break;

    case AST_EXPR_STATEMENT:
        if (node->data.expr)
            free_ast(node->data.expr);
        break;

    case AST_MATCH:
        free_ast(node->data.match.discriminant);
        for (int i = 0; i < node->data.match.case_count; i++)
        {
            free_ast(node->data.match.cases[i].value); // could be NULL
            free_ast(node->data.match.cases[i].body);
        }
        free(node->data.match.cases);
        break;

    case AST_MULTI_LET:
        for (int i = 0; i < node->data.multilet.count; i++)
        {
            free_ast(node->data.multilet.declarations[i]);
        }
        free(node->data.multilet.declarations);
        break;
    case AST_INPUT:
        free_ast(node->data.prompt);
        break;
    default:
        break;
    }
    free(node);
}