#include "codegen/codegen.h"
#include "symtab/symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static void emit_statement(ASTNode *node, FILE *out, int indent);
static void emit_expression(ASTNode *node, FILE *out);

// Returns the Quasar variable type of an expression node.
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
        if (op == OP_EQ || op == OP_NE || op == OP_LT || op == OP_GT || op == OP_LE || op == OP_GE || op == OP_AND || op == OP_OR)
        {
            return TYPE_BOOL;
        }

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

        if (op == OP_ASSIGN)
        {
            return infer_type(node->data.binary.left); // type of left side
        }

        // compound assignment yields the type of the left operand
        if (op == OP_ADD_ASSIGN || op == OP_SUB_ASSIGN || op == OP_MUL_ASSIGN ||
            op == OP_DIV_ASSIGN || op == OP_MOD_ASSIGN || op == OP_POW_ASSIGN ||
            op == OP_FLDIV_ASSIGN)
        {
            return infer_type(node->data.binary.left);
        }

        return TYPE_INT; // fallback
    }
    case AST_UNARY:
        if (node->data.unary.op == UNARY_NOT)
            return TYPE_BOOL;
        if (node->data.unary.op == UNARY_PRE_INC || node->data.unary.op == UNARY_PRE_DEC ||
            node->data.unary.op == UNARY_POST_INC || node->data.unary.op == UNARY_POST_DEC)
            return infer_type(node->data.unary.operand);
        return TYPE_INT;

    case AST_INPUT:
        return TYPE_STRING;

    case AST_TYPE_CONV:
        return node->data.typeconv.target;

    default:
        return TYPE_INT; // <- caused mental damage
    }
}

static const char *op_to_cstring(BinaryOp op)
{
    switch (op)
    {
    case OP_ADD:
        return "+";
    case OP_SUB:
        return "-";
    case OP_MUL:
        return "*";
    case OP_DIV:
        return "/";
    case OP_MOD:
        return "%";
    case OP_POW:
        return "pow"; // we'll emit pow()
    case OP_FLDIV:
        return "/"; // floor division in C is just / with ints; but we'll handle type later
    case OP_EQ:
        return "==";
    case OP_NE:
        return "!=";
    case OP_LT:
        return "<";
    case OP_GT:
        return ">";
    case OP_LE:
        return "<=";
    case OP_GE:
        return ">=";
    case OP_AND:
        return "&&";
    case OP_OR:
        return "||";
    case OP_ASSIGN:
        return "=";
    case OP_ADD_ASSIGN:
        return "+=";
    case OP_SUB_ASSIGN:
        return "-=";
    case OP_MUL_ASSIGN:
        return "*=";
    case OP_DIV_ASSIGN:
        return "/=";
    case OP_MOD_ASSIGN:
        return "%=";
    case OP_POW_ASSIGN:
        return "**=";
    case OP_FLDIV_ASSIGN:
        return "//=";
    default:
        return "???";
    }
}

static const char *format_spec_for(ASTNode *node)
{
    if (node->type == AST_VARIABLE)
    {
        // Directly look up the variable's type from the symbol table
        VarType t = symtab_lookup(node->data.varName);
        switch (t)
        {
        case TYPE_INT:
            return "%d";
        case TYPE_FLOAT:
            return "%g";
        case TYPE_STRING:
            return "%s";
        case TYPE_CHAR:
            return "%c";
        case TYPE_BOOL:
            return "%s"; // true/false as string
        default:
            return "%d";
        }
    }
    else
    {
        // For literals and other expressions, use infer_type
        VarType type = infer_type(node);
        switch (type)
        {
        case TYPE_INT:
            return "%d";
        case TYPE_FLOAT:
            return "%g";
        case TYPE_STRING:
            return "%s";
        case TYPE_CHAR:
            return "%c";
        case TYPE_BOOL:
            return "%s";
        default:
            return "%d";
        }
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

    /* Standard headers */
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <string.h>\n");
    fprintf(out, "#include <stdbool.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <math.h>\n\n");

    /* ========== Quasar runtime helpers (global scope) ========== */
    fprintf(out, "/* Quasar runtime helpers */\n");

    fprintf(out, "char *quasar_input(const char *prompt) {\n");
    fprintf(out, "    if (prompt) printf(\"%%s\", prompt);\n");
    fprintf(out, "    char buffer[1024];\n");
    fprintf(out, "    if (fgets(buffer, sizeof(buffer), stdin)) {\n");
    fprintf(out, "        size_t len = strlen(buffer);\n");
    fprintf(out, "        if (len > 0 && buffer[len-1] == '\\n') buffer[len-1] = '\\0';\n");
    fprintf(out, "        return strdup(buffer);\n");
    fprintf(out, "    }\n");
    fprintf(out, "    return strdup(\"\");\n");
    fprintf(out, "}\n\n");

    fprintf(out, "char *quasar_to_string(double x) {\n");
    fprintf(out, "    char buffer[128];\n");
    fprintf(out, "    snprintf(buffer, sizeof(buffer), \"%%g\", x);\n");
    fprintf(out, "    return strdup(buffer);\n");
    fprintf(out, "}\n\n");
    /* ======================================================== */

    /* Main function */
    fprintf(out, "int main(void) {\n");

    // Emit all statements (no more helpers here!)
    for (int i = 0; i < program->data.program.count; i++)
    {
        emit_statement(program->data.program.statements[i], out, 1);
    }

    fprintf(out, "\treturn 0;\n");
    fprintf(out, "}\n");
}

static void emit_expression_maybe_bool(ASTNode *node, FILE *out) // boolean helper
{
    if (infer_type(node) == TYPE_BOOL)
    {
        fprintf(out, "((");
        emit_expression(node, out);
        fprintf(out, ") ? \"true\" : \"false\")");
    }
    else
    {
        emit_expression(node, out);
    }
}

static void emit_statement(ASTNode *node, FILE *out, int indent_level)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case AST_PRINT:
    {
        indent(out, indent_level);
        int n = node->data.print.count;
        if (n == 0)
        {
            fprintf(out, "printf(\"\\n\");\n");
            break;
        }

        // Build dynamic format string using format_spec_for
        fprintf(out, "printf(\"");
        for (int i = 0; i < n; i++)
        {
            fprintf(out, "%s", format_spec_for(node->data.print.expressions[i]));
            if (i < n - 1)
                fprintf(out, " ");
        }
        fprintf(out, "\\n\"");

        // Emit arguments, using the bool‑friendly wrapper
        for (int i = 0; i < n; i++)
        {
            fprintf(out, ", ");
            emit_expression_maybe_bool(node->data.print.expressions[i], out);
        }
        fprintf(out, ");\n");
        break;
    }

    case AST_LET:
        indent(out, indent_level);
        fprintf(out, "%s %s", ctype_string(node->data.let.vartype),
                node->data.let.name);
        if (node->data.let.init)
        {
            fprintf(out, " = ");
            emit_expression(node->data.let.init, out);
        }
        fprintf(out, ";\n");
        break;
        break;

    case AST_ASSIGN:
        indent(out, indent_level);
        fprintf(out, "%s = ", node->data.assign.name);
        emit_expression(node->data.assign.value, out);
        fprintf(out, ";\n");
        break;

    case AST_BLOCK:
        indent(out, indent_level);
        fprintf(out, "{\n");
        for (int i = 0; i < node->data.block.count; i++)
        {
            emit_statement(node->data.block.statements[i], out, indent_level + 1);
        }
        indent(out, indent_level);
        fprintf(out, "}\n");
        break; // <-- gave me mental torture

    case AST_IF:
    {
        ASTNode *current = node;
        bool first = true;
        while (current)
        {
            if (current->data.ifelse.condition)
            {
                indent(out, indent_level);
                fprintf(out, "%s (", first ? "if" : "else if");
                emit_expression(current->data.ifelse.condition, out);
                fprintf(out, ")\n");
                emit_statement(current->data.ifelse.body, out, indent_level);
            }
            else
            {
                indent(out, indent_level);
                fprintf(out, "else\n");
                emit_statement(current->data.ifelse.body, out, indent_level);
            }
            first = false;
            current = current->data.ifelse.next;
        }
        break;
    }

    case AST_WHILE:
    {
        indent(out, indent_level);
        fprintf(out, "while (");
        emit_expression(node->data.whileloop.condition, out);
        fprintf(out, ")\n");
        emit_statement(node->data.whileloop.body, out, indent_level);
        break;
    }

    case AST_REPEAT_UNTIL:
    {
        indent(out, indent_level);
        fprintf(out, "do\n");
        emit_statement(node->data.repeatuntil.body, out, indent_level);
        indent(out, indent_level);
        fprintf(out, "while (!(");
        emit_expression(node->data.repeatuntil.condition, out);
        fprintf(out, "));\n");
        break;
    }

    case AST_FOR:
    {
        fprintf(out, "for (");
        // init
        if (node->data.forloop.init)
        {
            if (node->data.forloop.init->type == AST_LET)
            {
                ASTNode *let_node = node->data.forloop.init;
                fprintf(out, "%s %s = ", ctype_string(let_node->data.let.vartype), let_node->data.let.name);
                emit_expression(let_node->data.let.init, out);
            }
            else
            {
                emit_expression(node->data.forloop.init, out);
            }
        }
        fprintf(out, "; ");
        // condition
        if (node->data.forloop.condition)
        {
            emit_expression(node->data.forloop.condition, out);
        }
        else
        {
            fprintf(out, "1");
        }
        fprintf(out, "; ");
        // update
        if (node->data.forloop.update)
        {
            emit_expression(node->data.forloop.update, out);
        }
        fprintf(out, ")\n");
        emit_statement(node->data.forloop.body, out, indent_level);
        break;
    }

    case AST_BREAK:
        indent(out, indent_level);
        fprintf(out, "break;\n");
        break;

    case AST_CONTINUE:
        indent(out, indent_level);
        fprintf(out, "continue;\n");
        break;

    case AST_MATCH:
    {
        indent(out, indent_level);
        fprintf(out, "switch (");
        emit_expression(node->data.match.discriminant, out);
        fprintf(out, ") {\n");

        for (int i = 0; i < node->data.match.case_count; i++)
        {
            ASTNode *value = node->data.match.cases[i].value;
            ASTNode *body = node->data.match.cases[i].body;

            if (value)
            {
                indent(out, indent_level);
                fprintf(out, "case ");
                // emit the value (should be a literal)
                emit_expression(value, out);
                fprintf(out, ":\n");
            }
            else
            {
                indent(out, indent_level);
                fprintf(out, "default:\n");
            }
            // emit the body statements with increased indent
            // body is an AST_BLOCK, which contains the statements; we can just call emit_statement on the block, which will handle the indentation.
            emit_statement(body, out, indent_level + 1);
        }

        indent(out, indent_level);
        fprintf(out, "}\n");
        break;
    }

    case AST_MULTI_LET:
    {
        for (int i = 0; i < node->data.multilet.count; i++)
        {
            // emit each declaration as a statement (they are AST_LET nodes)
            emit_statement(node->data.multilet.declarations[i], out, indent_level);
        }
        break;
    }

    case AST_EXPR_STATEMENT:
        indent(out, indent_level);
        emit_expression(node->data.expr, out);
        fprintf(out, ";\n");
        break;

    case AST_RETURN:
        indent(out, indent_level);
        if (node->data.return_expr)
        {
            fprintf(out, "return ");
            emit_expression(node->data.return_expr, out);
            fprintf(out, ";\n");
        }
        else
        {
            fprintf(out, "return;\n");
        }
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
    case AST_BINARY:
    {
        // Special case for exponent: use pow() function
        if (node->data.binary.op == OP_POW)
        {
            fprintf(out, "pow(");
            emit_expression(node->data.binary.left, out);
            fprintf(out, ", ");
            emit_expression(node->data.binary.right, out);
            fprintf(out, ")");
        }
        else
        {
            fprintf(out, "(");
            emit_expression(node->data.binary.left, out);
            fprintf(out, " %s ", op_to_cstring(node->data.binary.op));
            emit_expression(node->data.binary.right, out);
            fprintf(out, ")");
        }
        break;
    }
    case AST_UNARY:
        if (node->data.unary.op == UNARY_NOT)
        {
            fprintf(out, "!(");
            emit_expression(node->data.unary.operand, out);
            fprintf(out, ")");
        }
        else if (node->data.unary.op == UNARY_MINUS)
        {
            fprintf(out, "-(");
            emit_expression(node->data.unary.operand, out);
            fprintf(out, ")");
        }
        else if (node->data.unary.op == UNARY_PLUS)
        {
            emit_expression(node->data.unary.operand, out);
        }
        else if (node->data.unary.op == UNARY_PRE_INC)
        {
            fprintf(out, "++(");
            emit_expression(node->data.unary.operand, out);
            fprintf(out, ")");
        }
        else if (node->data.unary.op == UNARY_PRE_DEC)
        {
            fprintf(out, "--(");
            emit_expression(node->data.unary.operand, out);
            fprintf(out, ")");
        }
        else if (node->data.unary.op == UNARY_POST_INC)
        {
            fprintf(out, "(");
            emit_expression(node->data.unary.operand, out);
            fprintf(out, ")++");
        }
        else if (node->data.unary.op == UNARY_POST_DEC)
        {
            fprintf(out, "(");
            emit_expression(node->data.unary.operand, out);
            fprintf(out, ")--");
        }
        break;
    case AST_INPUT:
        if (node->data.prompt)
        {
            fprintf(out, "quasar_input(");
            emit_expression(node->data.prompt, out);
            fprintf(out, ")");
        }
        else
        {
            fprintf(out, "quasar_input(NULL)");
        }
        break;
    case AST_TYPE_CONV:
    {
        VarType target = node->data.typeconv.target;
        ASTNode *arg = node->data.typeconv.source;

        switch (target)
        {
        case TYPE_INT:
            fprintf(out, "atoi(");
            emit_expression(arg, out);
            fprintf(out, ")");
            break;
        case TYPE_FLOAT:
            fprintf(out, "atof(");
            emit_expression(arg, out);
            fprintf(out, ")");
            break;
        case TYPE_STRING:
            // If arg is already string, just emit it; else use helper
            if (infer_type(arg) == TYPE_STRING)
            {
                emit_expression(arg, out);
            }
            else
            {
                fprintf(out, "quasar_to_string(");
                emit_expression(arg, out);
                fprintf(out, ")");
            }
            break;
        case TYPE_CHAR:
            // If arg is int, just use (char); if string, take first char
            if (infer_type(arg) == TYPE_INT)
            {
                fprintf(out, "(char)(");
                emit_expression(arg, out);
                fprintf(out, ")");
            }
            else
            {
                // string: take first char, else '\\0'
                fprintf(out, "(");
                emit_expression(arg, out);
                fprintf(out, ")[0]");
            }
            break;
        case TYPE_BOOL:
            // If arg is string, compare to "true"? Simpler: emit (strcmp(arg, "true")==0 || atoi(arg)!=0)
            if (infer_type(arg) == TYPE_STRING)
            {
                fprintf(out, "((strcmp(");
                emit_expression(arg, out);
                fprintf(out, ", \"true\") == 0) || (atoi(");
                emit_expression(arg, out);
                fprintf(out, ") != 0))");
            }
            else if (infer_type(arg) == TYPE_BOOL)
            {
                emit_expression(arg, out);
            }
            else
            {
                fprintf(out, "(");
                emit_expression(arg, out);
                fprintf(out, " != 0)");
            }
            break;
        }
        break;
    }
    default:
        break;
    }
}
