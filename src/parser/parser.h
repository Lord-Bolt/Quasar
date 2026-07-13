#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

int parser_error_count(void);
ASTNode *parse_program(const char *source);

#endif