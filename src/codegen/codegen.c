#include "codegen/codegen.h"
#include <stdio.h>
#include <stdlib.h>

static void emit_statement(ASTNode *node, FILE *out, int indent);
static void emit_expression(ASTNode *node, FILE *out);

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
    for (int i = 0; i < program->data.program.count; i++)
    {
        emit_statement(program->data.program.statements[i], out, 1);
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
        ASTNode *expr = node->data.expr;
        if (expr->type == AST_INTEGER)
        {
            fprintf(out, "printf(\"%%d\\n\", ");
        }
        else if (expr->type == AST_FLOAT)
        {
            fprintf(out, "printf(\"%%g\\n\", ");
        }
        else if (expr->type == AST_STRING)
        {
            fprintf(out, "printf(\"%%s\\n\", ");
        }
        else
        {
            fprintf(stderr, "Unknown expression type in print\n");
            exit(1);
        }
        emit_expression(expr, out);
        fprintf(out, ");\n");
        break;
    }

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
    default:
        break;
    }
}