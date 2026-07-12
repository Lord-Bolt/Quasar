#ifndef SYMTAB_H
#define SYMTAB_H

typedef enum
{
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_CHAR,
    TYPE_BOOL
} VarType;

void symtab_add(const char *name, VarType type);
VarType symtab_lookup(const char *name);
const char *ctype_string(VarType type);
const char *ctype_spec_string(VarType type);

#endif