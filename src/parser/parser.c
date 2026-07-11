#include "parser/parser.h"
#include "lexer/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

const char *token_name(QTokenType type)
{
    switch (type)
    {
    case QTOKEN_PRINT:
        return "'print'";
    case QTOKEN_LPAREN:
        return "'('";
    case QTOKEN_RPAREN:
        return "')'";
    case QTOKEN_INTEGER:
        return "integer literal";
    case QTOKEN_FLOAT:
        return "float literal";
    case QTOKEN_STRING:
        return "string literal";
    case QTOKEN_SEMICOLON:
        return "';'";
    case QTOKEN_EOF:
        return "end of file";
    case QTOKEN_UNKNOWN:
        return "unknown token";
    default:
        return "???";
    }
}

static void free_token_string(Token *token)
{
    if (token->str)
    {
        free(token->str);
        token->str = NULL;
    }
}

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

// Before: static void expect(QTokenType type, const char *errmsg)
static bool expect(QTokenType type, const char *context)
{
    if (!match(type))
    {
        fprintf(stderr, "Parse error: expected %s", token_name(type));
        if (context)
            fprintf(stderr, " (%s)", context);
        fprintf(stderr, ", but got %s\n", token_name(g_current.type));
        return false;
    }
    return true;
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
    else if (g_current.type == QTOKEN_FLOAT)
    {
        double val = g_current.floatValue;
        advance();
        return make_float(val);
    }
    else if (g_current.type == QTOKEN_STRING)
    {
        char *str = g_current.str; // take ownership
        advance();                 // consume token
        ASTNode *node = make_string(str);
        free(str); // since make_string already strdup'd it
        return node;
    }
    // Replace the old error line with:
    fprintf(stderr, "Parser error: expected expression, but got %s\n",
            token_name(g_current.type));
    return NULL;
}

static ASTNode *parse_print_statement(void)
{
    advance(); // consume 'print'
    if (!expect(QTOKEN_LPAREN, "after 'print'"))
        return NULL;
    ASTNode *expr = parse_expression();
    if (!expr)
        return NULL; // <-- bail out immediately
    if (!expect(QTOKEN_RPAREN, "after expression"))
    {
        free_ast(expr); // clean up the expression
        return NULL;
    }
    if (!expect(QTOKEN_SEMICOLON, "after print statement"))
    {
        free_ast(expr);
        return NULL;
    }
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
        fprintf(stderr, "Parser error: unexpected token %s at start of statement\n",
                token_name(g_current.type));
        return NULL;
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
        // Skip stray unknown tokens
        if (g_current.type == QTOKEN_UNKNOWN)
        {
            free_token_string(&g_current);
            advance();
            continue;
        }

        ASTNode *stmt = parse_statement();
        if (stmt)
        {
            // Success – add it to the program
            program_add_statement(program, stmt);
        }
        else
        {
            // Syntax error – skip to next statement
            fprintf(stderr, "Warning: skipping malformed statement\n");
            // Go to the start of the next statement (after the next semicolon)
            while (g_current.type != QTOKEN_SEMICOLON && g_current.type != QTOKEN_EOF)
            {
                free_token_string(&g_current);
                advance();
            }
            if (g_current.type == QTOKEN_SEMICOLON)
            {
                advance(); // consume the semicolon
            }
            // The loop will now try the next statement
        }
    }
    return program;
}
