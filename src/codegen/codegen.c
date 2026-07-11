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
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "int main(void) {\n");
    emit_statement(program, out, 1);
    fprintf(out, "  return 0;\n");
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
        // print( <expr> ) → printf("%d\n", <expr>);
        // For integers: %d, later we'll check expression type for %s, etc.
        indent(out, indent_level);
        fprintf(out, "printf(\"%%d\\n\", "); // emit expr recursively
        emit_expression(node->data.expr, out);
        fprintf(out, ");\n");
        break;

    default:
        fprintf(stderr, "Error : unknown statement type %d\n", node->type);
        exit(1);
    }
}

static void emit_expression(ASTNode *node, FILE *out)
{
    if (node == NULL)
        return;
    switch (node->type)
    {
    case AST_INTEGER:
        fprintf(out, "%d", node->data.intValue);
        break;
    default:
        fprintf(stderr, "Error: unknown expression type %d\n", node->type);
        exit(1);
    }
}