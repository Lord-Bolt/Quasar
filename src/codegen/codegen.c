#include "codegen/codegen.h"
#include "symtab/symtab.h"
#include <stdio.h>
#include <stdlib.h>

static void emit_statement(ASTNode *node, FILE *out, int indent);
static void emit_expression(ASTNode *node, FILE *out);

static const char *format_spec_for(ASTNode *node)
{
    switch (node->type)
    {
    case AST_INTEGER:
        return "%d";
    case AST_FLOAT:
        return "%g";
    case AST_STRING:
        return "%s";
    case AST_CHAR:
        return "%c";
    case AST_BOOL:
        return "%d";
    case AST_VARIABLE: // Look up the variable's type in the symbol table
        return ctype_spec_string(symtab_lookup(node->data.varName));
    default:
        return "???"; // error
    }
}

static void indent(FILE *out, int level)
{
    for (int i = 0; i < level; i++)
    {
        fprintf(out, "  ");
    }
}

void generate_code(ASTNode *program, FILE *out)
{
    if (program->type != AST_PROGRAM)
    {
        fprintf(stderr, "Error: root node must be AST_PROGRAM\n");
        exit(1);
    }
    fprintf(out, "#include <stdio.h>\n\n");
    fprintf(out, "int main(void) {\n");

    // First pass: emit all variable declarations (AST_LET)
    for (int i = 0; i < program->data.program.count; i++)
    {
        ASTNode *stmt = program->data.program.statements[i];
        if (stmt->type == AST_LET)
        {
            fprintf(out, "    %s %s", ctype_string(stmt->data.let.vartype), stmt->data.let.name);
            if (stmt->data.let.init)
            {
                fprintf(out, " = ");
                emit_expression(stmt->data.let.init, out);
            }
            fprintf(out, ";\n");
        }
    }

    // Second pass: emit all other statements (print, etc.)
    for (int i = 0; i < program->data.program.count; i++)
    {
        ASTNode *stmt = program->data.program.statements[i];
        if (stmt->type != AST_LET)
        {
            emit_statement(stmt, out, 1);
        }
    }

    fprintf(out, "    return 0;\n");
    fprintf(out, "}\n");
}

// Recursively emit code for a statement (for now only print)
static void emit_statement(ASTNode *node, FILE *out, int indent_level)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case AST_PRINT:
    {
        indent(out, indent_level);
        // Build format string and argument list
        int n = node->data.print.count;
        if (n == 0)
        {
            fprintf(out, "printf(\"\\n\");\n"); // just a newline
            break;
        }

        // write the format string with separators (space) between args, then newline
        fprintf(out, "printf(\"");
        for (int i = 0; i < n; i++)
        {
            fprintf(out, "%s", format_spec_for(node->data.print.expressions[i]));
            if (i < n - 1)
                fprintf(out, " "); // optional space between arguments
        }
        fprintf(out, "\\n\"");

        // Now the arguments
        for (int i = 0; i < n; i++)
        {
            fprintf(out, ", ");
            emit_expression(node->data.print.expressions[i], out);
        }
        fprintf(out, ");\n");
        break;
    }

    case AST_LET:
        // already handled in declarations
        break;

    case AST_ASSIGN:
        indent(out, indent_level);
        fprintf(out, "%s = ", node->data.assign.name);
        emit_expression(node->data.assign.value, out);
        fprintf(out, ";\n");
        break;

    default:
        fprintf(stderr, "Error : unknown statement type %d\n", node->type);
        exit(1);
    }
}

static void emit_expression(ASTNode *node, FILE *out)
{
    switch (node->type)
    {
    case AST_INTEGER:
        fprintf(out, "%d", node->data.intValue);
        break;
    case AST_FLOAT:
        fprintf(out, "%f", node->data.floatValue);
        break;
    case AST_STRING:
    {
        fputc('"', out);
        for (char *p = node->data.strValue; *p; p++)
        {
            switch (*p)
            {
            case '\n':
                fputs("\\n", out);
                break;
            case '\t':
                fputs("\\t", out);
                break;
            case '\r':
                fputs("\\r", out);
                break;
            case '\\':
                fputs("\\\\", out);
                break;
            case '"':
                fputs("\\\"", out);
                break;
            case '\0':
                fputs("\\0", out);
                break; // null character
            case '\a':
                fputs("\\a", out);
                break; // alert (bell)
            case '\b':
                fputs("\\b", out);
                break; // backspace
            case '\f':
                fputs("\\f", out);
                break; // form feed
            case '\v':
                fputs("\\v", out);
                break; // vertical tab
            default:
                fputc(*p, out);
                break;
            }
        }
        fputc('"', out);
        break;
    }
    case AST_CHAR:
    {
        // Emit as a C char literal, escaping if needed
        fputc('\'', out);
        char c = node->data.charValue;
        switch (c)
        {
        case '\n':
            fputs("\\n", out);
            break;
        case '\t':
            fputs("\\t", out);
            break;
        case '\r':
            fputs("\\r", out);
            break;
        case '\\':
            fputs("\\\\", out);
            break;
        case '\'':
            fputs("\\'", out);
            break;
        case '\0':
            fputs("\\0", out);
            break;
        case '\a':
            fputs("\\a", out);
            break;
        case '\b':
            fputs("\\b", out);
            break;
        case '\f':
            fputs("\\f", out);
            break;
        case '\v':
            fputs("\\v", out);
            break;
        default:
            fputc(c, out);
            break;
        }
        fputc('\'', out);
        break;
    }
    case AST_BOOL:
        fprintf(out, "%d", node->data.boolValue);
        break;
    case AST_VARIABLE:
        fprintf(out, "%s", node->data.varName);
        break;
    default:
        break;
    }
}