#define _POSIX_C_SOURCE 200809L
#include "parser/parser.h"
#include "lexer/lexer.h"
#include "symtab/symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static ASTNode *parse_expression(void);
static ASTNode *parse_statement(void);
static ASTNode *parse_primary(void);
static ASTNode *parse_unary(void);
static ASTNode *parse_relational(void);
static ASTNode *parse_logical_and(void);
static ASTNode *parse_logical_or(void);
static ASTNode *parse_block(void);
static ASTNode *parse_if_statement(void);
static ASTNode *parse_while_statement(void);
static ASTNode *parse_repeat_until_statement(void);
static ASTNode *parse_postfix(void);

static int parse_errors = 0;

static VarType infer_type(ASTNode *node)
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

    case AST_BINARY:
    {
        VarType left = infer_type(node->data.binary.left);
        VarType right = infer_type(node->data.binary.right);
        BinaryOp op = node->data.binary.op;
        if (op == OP_POW)
            return TYPE_FLOAT;

        // Arithmetic operators: if either operand is float, result is float; otherwise int.
        if (op == OP_ADD || op == OP_SUB || op == OP_MUL || op == OP_DIV ||
            op == OP_MOD || op == OP_FLDIV)
        {
            if (left == TYPE_FLOAT || right == TYPE_FLOAT)
                return TYPE_FLOAT;
            return TYPE_INT; // both ints -> int
        }
        // For future operators (relational, logical), they will return TYPE_BOOL (int).
        // We'll handle them when we add those operators.
        return TYPE_INT; // fallback
    }

    default:
        return TYPE_INT; // safe fallback for any unknown node
    }
}

static bool check_unary_types(UnaryOp op, ASTNode *operand)
{
    VarType t = infer_type(operand);
    if (op == UNARY_NOT)
    {
        if (t != TYPE_INT && t != TYPE_FLOAT && t != TYPE_BOOL)
        {
            fprintf(stderr, "Error: invalid operand type for '!': %s\n", ctype_string(t));
            parse_errors++;
            return false;
        }
    }
    else if (op == UNARY_MINUS || op == UNARY_PLUS)
    {
        if (t != TYPE_INT && t != TYPE_FLOAT)
        {
            fprintf(stderr, "Error: invalid operand type for unary '%c': %s\n",
                    op == UNARY_MINUS ? '-' : '+', ctype_string(t));
            parse_errors++;
            return false;
        }
    }
    else if (op == UNARY_PRE_INC || op == UNARY_PRE_DEC ||
             op == UNARY_POST_INC || op == UNARY_POST_DEC)
    {
        // Must be a variable (identifier) and numeric.
        if (operand->type != AST_VARIABLE)
        {
            fprintf(stderr, "Error: increment/decrement operand must be a variable\n");
            parse_errors++;
            return false;
        }
        if (t != TYPE_INT && t != TYPE_FLOAT)
        {
            fprintf(stderr, "Error: invalid operand type for ++/--: %s\n", ctype_string(t));
            parse_errors++;
            return false;
        }
    }
    return true;
}

// Returns true if a value of type `source` can be assigned to a variable of type `target`.
static bool compatible_assignment(VarType target, VarType source)
{
    if (target == source)
        return true;
    // Allow implicit int → float
    if (target == TYPE_FLOAT && source == TYPE_INT)
        return true;
    return false;
}

static bool valid_binary_types(VarType left, VarType right, BinaryOp op)
{
    switch (op)
    {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
    case OP_POW:
    case OP_FLDIV:
        return (left == TYPE_INT || left == TYPE_FLOAT) &&
               (right == TYPE_INT || right == TYPE_FLOAT);
    case OP_EQ:
    case OP_NE:
        // Allow int/float/bool mixing (bool is now a distinct type but comparable)
        return (left == TYPE_INT || left == TYPE_FLOAT || left == TYPE_BOOL) &&
               (right == TYPE_INT || right == TYPE_FLOAT || right == TYPE_BOOL);
    case OP_LT:
    case OP_GT:
    case OP_LE:
    case OP_GE:
        return (left == TYPE_INT || left == TYPE_FLOAT) &&
               (right == TYPE_INT || right == TYPE_FLOAT);
    case OP_AND:
    case OP_OR:
        return (left == TYPE_INT || left == TYPE_FLOAT || left == TYPE_BOOL) &&
               (right == TYPE_INT || right == TYPE_FLOAT || right == TYPE_BOOL);
    default:
        return false;
    }
}

static bool check_binary_types(BinaryOp op, ASTNode *left, ASTNode *right)
{
    VarType ltype = infer_type(left);
    VarType rtype = infer_type(right);
    if (!valid_binary_types(ltype, rtype, op))
    {
        fprintf(stderr, "Error: invalid operand types for ");
        // Determine if it's arithmetic or relational based on op
        bool is_relational = (op == OP_EQ || op == OP_NE || op == OP_LT || op == OP_GT || op == OP_LE || op == OP_GE);
        fprintf(stderr, "%s operator (", is_relational ? "relational" : "arithmetic");
        switch (op)
        {
        case OP_ADD:
            fprintf(stderr, "+");
            break;
        case OP_SUB:
            fprintf(stderr, "-");
            break;
        case OP_MUL:
            fprintf(stderr, "*");
            break;
        case OP_DIV:
            fprintf(stderr, "/");
            break;
        case OP_MOD:
            fprintf(stderr, "%%");
            break;
        case OP_POW:
            fprintf(stderr, "**");
            break;
        case OP_FLDIV:
            fprintf(stderr, "//");
            break;
        case OP_EQ:
            fprintf(stderr, "==");
            break;
        case OP_NE:
            fprintf(stderr, "!=");
            break;
        case OP_LT:
            fprintf(stderr, "<");
            break;
        case OP_GT:
            fprintf(stderr, ">");
            break;
        case OP_LE:
            fprintf(stderr, "<=");
            break;
        case OP_GE:
            fprintf(stderr, ">=");
            break;
        default:
            fprintf(stderr, "?");
            break;
        }
        fprintf(stderr, "): %s and %s\n", ctype_string(ltype), ctype_string(rtype));
        parse_errors++;
        return false;
    }
    return true;
}

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
    case QTOKEN_AND:
        return "'&&'";
    case QTOKEN_OR:
        return "'||'";
    case QTOKEN_NOT:
        return "'!'";
    case QTOKEN_IF:
        return "'if'";
    case QTOKEN_ELIF:
        return "'elif'";
    case QTOKEN_ELSE:
        return "'else'";
    case QTOKEN_WHILE:
        return "'while'";
    case QTOKEN_REPEAT:
        return "'repeat'";
    case QTOKEN_UNTIL:
        return "'until'";
    case QTOKEN_INC:
        return "'++'";
    case QTOKEN_DEC:
        return "'--'";
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
static ASTNode *parse_postfix(void)
{
    ASTNode *node = parse_primary(); // get the operand (identifier, literal, etc.)
    while (g_current.type == QTOKEN_INC || g_current.type == QTOKEN_DEC)
    {
        UnaryOp op = (g_current.type == QTOKEN_INC) ? UNARY_POST_INC : UNARY_POST_DEC;
        advance(); // consume '++' or '--'
        node = make_unary(op, node);
        // Type check the postfix operator
        if (!check_unary_types(op, node->data.unary.operand))
        {
            free_ast(node);
            return NULL;
        }
    }
    return node;
}

static ASTNode *parse_if_statement(void)
{
    advance(); // consume 'if'

    if (!expect(QTOKEN_LPAREN, "expected '(' after 'if'"))
        return NULL;

    ASTNode *condition = parse_expression();
    if (!condition)
        return NULL;

    if (!expect(QTOKEN_RPAREN, "expected ')' after condition"))
    {
        free_ast(condition);
        return NULL;
    }

    if (g_current.type != QTOKEN_LBRACE)
    {
        fprintf(stderr, "Error: expected '{' for if body (blocks are required)\n");
        parse_errors++;
        free_ast(condition);
        return NULL;
    }

    ASTNode *body = parse_block();
    if (!body)
    {
        free_ast(condition);
        return NULL;
    }

    ASTNode *current_if = make_if(condition, body, NULL);
    ASTNode *head = current_if;

    while (g_current.type == QTOKEN_ELIF || g_current.type == QTOKEN_ELSE)
    {
        if (g_current.type == QTOKEN_ELIF)
        {
            advance(); // consume 'elif'

            if (!expect(QTOKEN_LPAREN, "expected '(' after 'elif'"))
            {
                free_ast(head);
                return NULL;
            }
            ASTNode *elif_condition = parse_expression();
            if (!elif_condition)
            {
                free_ast(head);
                return NULL;
            }
            if (!expect(QTOKEN_RPAREN, "expected ')' after elif condition"))
            {
                free_ast(elif_condition);
                free_ast(head);
                return NULL;
            }
            if (g_current.type != QTOKEN_LBRACE)
            {
                fprintf(stderr, "Error: expected '{' for elif body (blocks are required)\n");
                parse_errors++;
                free_ast(elif_condition);
                free_ast(head);
                return NULL;
            }
            ASTNode *elif_body = parse_block();
            if (!elif_body)
            {
                free_ast(elif_condition);
                free_ast(head);
                return NULL;
            }

            ASTNode *elif_node = make_if(elif_condition, elif_body, NULL);
            current_if->data.ifelse.next = elif_node;
            current_if = elif_node;
        }
        else if (g_current.type == QTOKEN_ELSE)
        {
            advance(); // consume 'else'

            if (g_current.type != QTOKEN_LBRACE)
            {
                fprintf(stderr, "Error: expected '{' for else body (blocks are required)\n");
                parse_errors++;
                free_ast(head);
                return NULL;
            }
            ASTNode *else_body = parse_block();
            if (!else_body)
            {
                free_ast(head);
                return NULL;
            }

            ASTNode *else_node = make_if(NULL, else_body, NULL);
            current_if->data.ifelse.next = else_node;
            break;
        }
    }

    return head;
}

static ASTNode *parse_while_statement(void)
{
    advance(); // consume 'while'

    if (!expect(QTOKEN_LPAREN, "expected '(' after 'while'"))
        return NULL;

    ASTNode *condition = parse_expression();
    if (!condition)
        return NULL;

    if (!expect(QTOKEN_RPAREN, "expected ')' after condition"))
    {
        free_ast(condition);
        return NULL;
    }

    if (g_current.type != QTOKEN_LBRACE)
    {
        fprintf(stderr, "Error: expected '{' for while body (blocks are required)\n");
        parse_errors++;
        free_ast(condition);
        return NULL;
    }

    ASTNode *body = parse_block();
    if (!body)
    {
        free_ast(condition);
        return NULL;
    }

    return make_while(condition, body);
}

static ASTNode *parse_repeat_until_statement(void)
{
    advance(); // consume 'repeat'

    if (g_current.type != QTOKEN_LBRACE)
    {
        fprintf(stderr, "Error: expected '{' for repeat body (blocks are required)\n");
        parse_errors++;
        return NULL;
    }

    ASTNode *body = parse_block();
    if (!body)
        return NULL;

    // Must have 'until'
    if (!expect(QTOKEN_UNTIL, "expected 'until' after repeat block"))
    {
        free_ast(body);
        return NULL;
    }

    if (!expect(QTOKEN_LPAREN, "expected '(' after 'until'"))
    {
        free_ast(body);
        return NULL;
    }

    ASTNode *condition = parse_expression();
    if (!condition)
    {
        free_ast(body);
        return NULL;
    }

    if (!expect(QTOKEN_RPAREN, "expected ')' after until condition"))
    {
        free_ast(condition);
        free_ast(body);
        return NULL;
    }

    // Optional semicolon after until for consistency
    if (!expect(QTOKEN_SEMICOLON, "expected ';' after until clause"))
    {
        free_ast(condition);
        free_ast(body);
        return NULL;
    }

    return make_repeat_until(condition, body);
}

static ASTNode *parse_unary(void)
{
    if (g_current.type == QTOKEN_NOT)
    {
        advance();
        ASTNode *operand = parse_unary();
        if (!operand)
            return NULL;
        ASTNode *node = make_unary(UNARY_NOT, operand);
        if (!check_unary_types(UNARY_NOT, operand))
        {
            free_ast(node);
            return NULL;
        }
        return node;
    }
    if (g_current.type == QTOKEN_MINUS)
    {
        advance();
        ASTNode *operand = parse_unary();
        if (!operand)
            return NULL;
        ASTNode *node = make_unary(UNARY_MINUS, operand);
        if (!check_unary_types(UNARY_MINUS, operand))
        {
            free_ast(node);
            return NULL;
        }
        return node;
    }
    if (g_current.type == QTOKEN_PLUS)
    {
        advance();
        ASTNode *operand = parse_unary();
        if (!operand)
            return NULL;
        ASTNode *node = make_unary(UNARY_PLUS, operand);
        if (!check_unary_types(UNARY_PLUS, operand))
        {
            free_ast(node);
            return NULL;
        }
        return node;
    }
    if (g_current.type == QTOKEN_INC)
    {
        advance();
        ASTNode *operand = parse_unary(); // recursive checking
        if (!operand)
            return NULL;
        ASTNode *node = make_unary(UNARY_PRE_INC, operand);
        if (!check_unary_types(UNARY_PRE_INC, operand))
        {
            free_ast(node);
            return NULL;
        }
        return node;
    }
    if (g_current.type == QTOKEN_DEC)
    {
        advance();
        ASTNode *operand = parse_unary();
        if (!operand)
            return NULL;
        ASTNode *node = make_unary(UNARY_PRE_DEC, operand);
        if (!check_unary_types(UNARY_PRE_DEC, operand))
        {
            free_ast(node);
            return NULL;
        }
        return node;
    }
    return parse_postfix();
}

static ASTNode *parse_logical_and(void)
{
    ASTNode *node = parse_relational();
    if (!node)
        return NULL;
    while (g_current.type == QTOKEN_AND)
    {
        advance();
        ASTNode *right = parse_relational();
        if (!right)
        {
            free_ast(node);
            return NULL;
        }
        node = make_binary(OP_AND, node, right);
        // type check
        if (!check_binary_types(OP_AND, node->data.binary.left, node->data.binary.right))
        {
            free_ast(node);
            return NULL;
        }
    }
    return node;
}

static ASTNode *parse_logical_or(void)
{
    ASTNode *node = parse_logical_and();
    if (!node)
        return NULL;
    while (g_current.type == QTOKEN_OR)
    {
        advance();
        ASTNode *right = parse_logical_and();
        if (!right)
        {
            free_ast(node);
            return NULL;
        }
        node = make_binary(OP_OR, node, right);
        if (!check_binary_types(OP_OR, node->data.binary.left, node->data.binary.right))
        {
            free_ast(node);
            return NULL;
        }
    }
    return node;
}

static ASTNode *parse_block(void)
{
    // Expect '{'
    if (!expect(QTOKEN_LBRACE, "expected '{' to start block"))
        return NULL;
    ASTNode *block = make_block();

    while (g_current.type != QTOKEN_RBRACE && g_current.type != QTOKEN_EOF)
    {
        ASTNode *stmt = parse_statement();
        if (!stmt)
        {
            // Error recovery inside block: skip to next '}' or statement-start
            while (g_current.type != QTOKEN_RBRACE && g_current.type != QTOKEN_EOF)
                advance();
            if (g_current.type == QTOKEN_RBRACE)
                advance();
            free_ast(block);
            return NULL;
        }
        block_add_statement(block, stmt);
    }

    if (!expect(QTOKEN_RBRACE, "expected '}' to close block"))
    {
        free_ast(block);
        return NULL;
    }
    return block;
}

int parser_error_count(void)
{
    return parse_errors;
}

static ASTNode *parse_primary(void)
{
    if (g_current.type == QTOKEN_INTEGER)
    {
        int val = g_current.value;
        advance();
        return make_integer(val);
    }
    if (g_current.type == QTOKEN_FLOAT)
    {
        double val = g_current.floatValue;
        advance();
        return make_float(val);
    }
    if (g_current.type == QTOKEN_STRING)
    {
        char *str = g_current.str;
        advance();
        ASTNode *node = make_string(str);
        free(str);
        return node;
    }
    if (g_current.type == QTOKEN_CHAR)
    {
        char val = (char)g_current.value;
        advance();
        return make_char(val);
    }
    if (g_current.type == QTOKEN_TRUE)
    {
        advance();
        return make_bool(1);
    }
    if (g_current.type == QTOKEN_FALSE)
    {
        advance();
        return make_bool(0);
    }
    if (g_current.type == QTOKEN_IDENTIFIER)
    {
        char *name = strdup(g_current.str);
        advance();
        return make_variable(name);
    }
    if (g_current.type == QTOKEN_LPAREN)
    {
        advance();                          // consume '('
        ASTNode *expr = parse_expression(); // will call the new top-level
        if (!expr)
            return NULL;
        if (!expect(QTOKEN_RPAREN, "expected ')' after expression"))
        {
            free_ast(expr);
            return NULL;
        }
        return expr;
    }
    fprintf(stderr, "Parse error: expected expression, but got %s\n", token_name(g_current.type));
    parse_errors++;
    return NULL;
}

static ASTNode *parse_assignment(char *name)
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
    if (!compatible_assignment(var_type, val_type))
    {
        fprintf(stderr, "Error: type mismatch in assignment to '%s' – expected %s but got %s\n",
                name, ctype_string(var_type), ctype_string(val_type));
        parse_errors++;
        free(name);
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
    if (!compatible_assignment(type, init_type))
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
    case QTOKEN_LBRACE:
        return parse_block();
    case QTOKEN_IF:
        return parse_if_statement();
    case QTOKEN_WHILE:
        return parse_while_statement();
    case QTOKEN_REPEAT:
        return parse_repeat_until_statement();
    default:
        fprintf(stderr, "Parser error: unexpected token %s at start of statement\n",
                token_name(g_current.type));
        parse_errors++;
        return NULL;
    }
}

static ASTNode *parse_multiplicative(void)
{
    ASTNode *node = parse_unary();
    if (!node)
        return NULL;
    while (g_current.type == QTOKEN_STAR || g_current.type == QTOKEN_SLASH ||
           g_current.type == QTOKEN_PERCENT || g_current.type == QTOKEN_POWER ||
           g_current.type == QTOKEN_FLOOR_DIV)
    {
        BinaryOp op;
        switch (g_current.type)
        {
        case QTOKEN_STAR:
            op = OP_MUL;
            break;
        case QTOKEN_SLASH:
            op = OP_DIV;
            break;
        case QTOKEN_PERCENT:
            op = OP_MOD;
            break;
        case QTOKEN_POWER:
            op = OP_POW;
            break;
        case QTOKEN_FLOOR_DIV:
            op = OP_FLDIV;
            break;
        default:
            break;
        }
        advance(); // consume operator
        ASTNode *right = parse_unary();
        if (!right)
        {
            free_ast(node);
            return NULL;
        }
        node = make_binary(op, node, right);
        if (!check_binary_types(op, node->data.binary.left, node->data.binary.right))
        {
            free_ast(node);
            return NULL;
        }
    }
    return node;
}

static ASTNode *parse_additive(void)
{
    ASTNode *node = parse_multiplicative();
    if (!node)
        return NULL;
    while (g_current.type == QTOKEN_PLUS || g_current.type == QTOKEN_MINUS)
    {
        BinaryOp op = (g_current.type == QTOKEN_PLUS) ? OP_ADD : OP_SUB;
        advance();
        ASTNode *right = parse_multiplicative();
        if (!right)
        {
            free_ast(node);
            return NULL;
        }
        node = make_binary(op, node, right);
        if (!check_binary_types(op, node->data.binary.left, node->data.binary.right))
        {
            free_ast(node);
            return NULL;
        }
    }
    return node;
}

static ASTNode *parse_relational(void)
{
    ASTNode *node = parse_additive();
    if (!node)
        return NULL;
    while (g_current.type == QTOKEN_EQ_EQ || g_current.type == QTOKEN_NOT_EQ ||
           g_current.type == QTOKEN_LT || g_current.type == QTOKEN_GT ||
           g_current.type == QTOKEN_LT_EQ || g_current.type == QTOKEN_GT_EQ)
    {
        BinaryOp op;
        switch (g_current.type)
        {
        case QTOKEN_EQ_EQ:
            op = OP_EQ;
            break;
        case QTOKEN_NOT_EQ:
            op = OP_NE;
            break;
        case QTOKEN_LT:
            op = OP_LT;
            break;
        case QTOKEN_GT:
            op = OP_GT;
            break;
        case QTOKEN_LT_EQ:
            op = OP_LE;
            break;
        case QTOKEN_GT_EQ:
            op = OP_GE;
            break;
        default:
            op = OP_ADD;
            break; // unreachable
        }
        advance();
        ASTNode *right = parse_additive();
        if (!right)
        {
            free_ast(node);
            return NULL;
        }
        node = make_binary(op, node, right);
        // Type-check the binary expression
        if (!check_binary_types(op, node->data.binary.left, node->data.binary.right))
        {
            free_ast(node);
            return NULL;
        }
    }
    return node;
}

static ASTNode *parse_expression(void)
{
    return parse_logical_or(); // lowest precedence;
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
            // Skip tokens until a recovery point
            while (g_current.type != QTOKEN_SEMICOLON &&
                   g_current.type != QTOKEN_EOF &&
                   g_current.type != QTOKEN_PRINT &&
                   g_current.type != QTOKEN_LET &&
                   g_current.type != QTOKEN_IDENTIFIER) // identifiers can begin assignment statements
            {
                advance();
            }
            if (g_current.type == QTOKEN_SEMICOLON)
            {
                advance();
            }
        }
    }
    return program;
}
