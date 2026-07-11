#include "parser/parser.h"
#include "lexer/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// --- Lexer interface ---
static const char *g_source;
static int g_pos;
static Token g_current;

static void advance(void)
{
    g_current = get_next_token(g_source, &g_pos);
}

static bool match(QTokenType type)
{
    if (g_current.type == type)
    {
        advance();
        return true;
    }
    return false;
}

static void expect(QTokenType type, const char *errmsg)
{
    if (!match(type))
    {
        fprintf(stderr, "Parser error : %s (expected %d, got %d)\n", errmsg, type, g_current.type);
        exit(1); // Later replacing with error recovery
    }
}

// ---Recursive Descent Fuctions---
static ASTNode *parse_expression(void)
{
    // For now, only int
    if (g_current.type == QTOKEN_INTEGER)
    {
        int val = g_current.value;
        advance();
        return make_integer(val);
    }
    else if (g_current.type == QTOKEN_STRING)
    {
        char *str = g_current.str; // take ownership
        advance();                 // consume token
        ASTNode *node = make_string(str);
        free(str); // since make_string already strdup'd it
        return node;
    }
    fprintf(stderr, "Expected expression, got token type %d\n", g_current.type);
    exit(1);
}

static ASTNode *parse_print_statement(void)
{
    advance(); // consume 'print'
    expect(QTOKEN_LPAREN, "expected '(' after 'print'");
    ASTNode *expr = parse_expression();
    expect(QTOKEN_RPAREN, "expected ')' after expression");
    expect(QTOKEN_SEMICOLON, "expected ';' after print statement");
    return make_print(expr);
}

static ASTNode *parse_statement(void)
{
    switch (g_current.type)
    {
    case QTOKEN_PRINT:
        return parse_print_statement();
    // TODO: case QTOKEN_LET: ...
    default:
        fprintf(stderr, "Unexpected token %d at start of statement\n", g_current.type);
        exit(1);
    }
}

ASTNode *parse_program(const char *source)
{
    g_source = source;
    g_pos = 0;
    advance();

    ASTNode *program = make_program();

    while (g_current.type != QTOKEN_EOF)
    {
        // Skip any unknown tokens (e.g., stray bytes at end of file)
        if (g_current.type == QTOKEN_UNKNOWN)
        {
            advance();
            continue;
        }
        ASTNode *stmt = parse_statement();
        if (!stmt)
        {
            free_ast(program);
            return NULL;
        }
        program_add_statement(program, stmt);
    }
    return program;
}
