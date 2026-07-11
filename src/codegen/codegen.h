#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser/ast.h"
#include <stdio.h>

void generate_code(ASTNode *program, FILE *out);

#endif