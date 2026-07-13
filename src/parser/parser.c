#define _POSIX_C_SOURCE 200809L
#include "parser/parser.h"
#include "lexer/lexer.h"
#include "symtab/symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static int parse_errors = 0;

static VarType expr_type(ASTNode *node)
{
    if (!node)
        return TYPE_INT; // safe default
    switch (node->type)
    {
    case AST_INTEGER:
        return TYPE_INT;
    case AST_FLOAT:
        return TYPE_FLOAT;
    case AST_STRING:
        return TYPE_STRING;
    case AST_CHAR:
        return TYPE_CHAR;
    case AST_BOOL:
        return TYPE_BOOL;
    case AST_VARIABLE:
        return symtab_lookup(node->data.varName);
    default:
        return TYPE_INT;
    }
}

static VarType token_to_vartype(QTokenType type)
{
    switch (type)
    {
    case QTOKEN_TYPE_INT:
        return TYPE_INT;
    case QTOKEN_TYPE_FLOAT:
        return TYPE_FLOAT;
    case QTOKEN_TYPE_STRING:
        return TYPE_STRING;
    case QTOKEN_TYPE_CHAR:
        return TYPE_CHAR;
    case QTOKEN_TYPE_BOOL:
        return TYPE_BOOL;
    default:
        return TYPE_INT; // error, but keep going
    }
}

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
    case QTOKEN_LET:
        return "'let'";
    case QTOKEN_COLON:
        return "':'";
    case QTOKEN_EQUALS:
        return "'='";
    case QTOKEN_IDENTIFIER:
        return "identifier";
    case QTOKEN_TYPE_INT:
        return "'int'";
    case QTOKEN_TYPE_FLOAT:
        return "'float'";
    case QTOKEN_TYPE_STRING:
        return "'string'";
    case QTOKEN_TYPE_CHAR:
        return "'char'";
    case QTOKEN_TYPE_BOOL:
        return "'bool'";
    case QTOKEN_EOF:
        return "end of file";
    case QTOKEN_UNKNOWN:
        return "unknown token";
    default:
        return "???";
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
        parse_errors++;
        return false;
    }
    return true;
}

// ---Recursive Descent Fuctions---
int parser_error_count(void)
{
    return parse_errors;
}

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
    else if (g_current.type == QTOKEN_CHAR)
    {
        char val = (char)g_current.value; // value holds the decoded character
        advance();
        return make_char(val);
    }
    else if (g_current.type == QTOKEN_IDENTIFIER)
    {
        char *name = strdup(g_current.str); // duplicate for AST
        advance();
        // symtab_lookup could be used later for type checking
        return make_variable(name);
    }
    else if (g_current.type == QTOKEN_TRUE)
    {
        advance();
        return make_bool(1);
    }
    else if (g_current.type == QTOKEN_FALSE)
    {
        advance();
        return make_bool(0);
    }

    fprintf(stderr, "Parser error: expected expression, but got %s\n",
            token_name(g_current.type));
    parse_errors++;
    return NULL;
}

static ASTNode *parse_assignment(const char *name)
{
    // name is the variable being assigned (already consumed from the token)
    // Current token should be '='
    if (!expect(QTOKEN_EQUALS, "expected '=' in assignment"))
    {
        return NULL;
    }
    ASTNode *value = parse_expression();
    if (!value)
    {
        return NULL;
    }
    if (!expect(QTOKEN_SEMICOLON, "expected ';' after assignment"))
    {
        free_ast(value);
        return NULL;
    }

    // Type check the new value
    VarType var_type = symtab_lookup(name);
    VarType val_type = expr_type(value);
    if (var_type != val_type)
    {
        fprintf(stderr, "Error: type mismatch in assignment to '%s' - expected %s but got %s\n",
                name, ctype_string(var_type), ctype_string(val_type));
        parse_errors++;
        free_ast(value);
        return NULL;
    }

    return make_assign(name, value);
}

static ASTNode *parse_print_statement(void)
{
    advance(); // consume 'print'
    if (!expect(QTOKEN_LPAREN, "expected '(' after 'print'"))
        return NULL;

    ASTNode *print_node = make_print_empty();

    // Check for empty argument list (maybe just a newline, we'll treat as error)
    if (g_current.type == QTOKEN_RPAREN)
    {
        fprintf(stderr, "Print statement requires at least one argument\n");
        free_ast(print_node);
        return NULL;
    }

    // Parse first expression
    ASTNode *expr = parse_expression();
    if (!expr)
    {
        free_ast(print_node);
        return NULL;
    }
    print_add_expression(print_node, expr);

    // Parse additional expressions separated by commas
    while (g_current.type == QTOKEN_COMMA)
    {
        advance(); // consume comma
        expr = parse_expression();
        if (!expr)
        {
            free_ast(print_node);
            return NULL;
        }
        print_add_expression(print_node, expr);
    }

    if (!expect(QTOKEN_RPAREN, "expected ')' after expression list"))
    {
        free_ast(print_node);
        return NULL;
    }
    // semicolon is optional? For now require it like before
    if (!expect(QTOKEN_SEMICOLON, "expected ';' after print statement"))
    {
        free_ast(print_node);
        return NULL;
    }
    return print_node;
}

static ASTNode *parse_let_statement(void)
{
    advance(); // consume 'let'

    // Expect variable name (identifier)
    if (g_current.type != QTOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Expected variable name after 'let'\n");
        return NULL;
    }
    char *name = strdup(g_current.str); // make a copy for the AST
    advance();                          // consume identifier

    if (!expect(QTOKEN_COLON, "expected ':' after variable name"))
    {
        free(name);
        return NULL;
    }

    // Expect a type keyword (int, float, string, char, bool)
    VarType type;
    if (g_current.type == QTOKEN_TYPE_INT || g_current.type == QTOKEN_TYPE_FLOAT ||
        g_current.type == QTOKEN_TYPE_STRING || g_current.type == QTOKEN_TYPE_CHAR ||
        g_current.type == QTOKEN_TYPE_BOOL)
    {
        type = token_to_vartype(g_current.type);
        advance();
    }
    else
    {
        fprintf(stderr, "Expected type after ':' in let statement\n");
        free(name);
        return NULL;
    }

    if (!expect(QTOKEN_EQUALS, "expected '=' in let statement"))
    {
        free(name);
        return NULL;
    }

    ASTNode *init = parse_expression();
    if (!init)
    {
        free(name);
        return NULL;
    }

    if (!expect(QTOKEN_SEMICOLON, "expected ';' after let statement"))
    {
        free_ast(init);
        free(name);
        return NULL;
    }

    // Register in symbol table
    symtab_add(name, type);

    // Type check the initializer
    VarType init_type = expr_type(init);
    if (init_type != type)
    {
        fprintf(stderr, "Error: type mismatch in 'let %s' - expected %s but got %s\n",
                name, ctype_string(type), ctype_string(init_type));
        parse_errors++;
        free(name);
        free_ast(init);
        return NULL;
    }

    return make_let(name, type, init);
}

static ASTNode *parse_statement(void)
{
    switch (g_current.type)
    {
    case QTOKEN_PRINT:
        return parse_print_statement();
    case QTOKEN_LET:
        return parse_let_statement();
    case QTOKEN_IDENTIFIER:
    {
        // Save the identifier name
        char *name = strdup(g_current.str);
        advance(); // consume identifier, Check if followed by '='
        if (g_current.type == QTOKEN_EQUALS)
        {
            ASTNode *assign = parse_assignment(name);
            free(name);
            return assign;
        }
        else
        {
            // Not an assignment; perhaps a function call later? For now, error.
            fprintf(stderr, "Unexpected token after identifier '%s' (expected '=' for assignment)\n", name);
            free(name);
            // Skip to next statement? We'll just return NULL to trigger recovery.
            return NULL;
        }
        break;
    }
    default:
        fprintf(stderr, "Parser error: unexpected token %s at start of statement\n",
                token_name(g_current.type));
        parse_errors++;
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
        ASTNode *stmt = parse_statement();
        if (stmt)
        {
            // Success – add it to the program
            program_add_statement(program, stmt);
        }
        else
        {
            // Syntax error – skip to next statement
            // Go to the start of the next statement (after the next semicolon)
            // Skip tokens until we reach a recovery point
            while (g_current.type != QTOKEN_SEMICOLON &&
                   g_current.type != QTOKEN_EOF &&
                   g_current.type != QTOKEN_PRINT &&
                   g_current.type != QTOKEN_LET &&
                   g_current.type != QTOKEN_IDENTIFIER) // identifiers can begin assignment statements
            {
                advance();
            }
            // If we stopped at a statement‑start token, we leave it for the next iteration.
            // If we stopped at a semicolon, consume it and then continue.
            if (g_current.type == QTOKEN_SEMICOLON)
            {
                advance();
            }
            // The loop will now try the next statement
        }
    }
    return program;
}
