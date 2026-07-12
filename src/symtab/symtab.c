#include "symtab.h"
#include <string.h>
#include <stdlib.h>

#define MAX_VARS 256

typedef struct
{
    char name[64];
    VarType type;
} Symbol;

static Symbol table[MAX_VARS]; // will make dynamic in future
static int count = 0;

void symtab_add(const char *name, VarType type)
{
    if (count < MAX_VARS)
    {
        strncpy(table[count].name, name, 63);
        table[count].name[63] = '\0';
        table[count].type = type;
        count++;
    }
}

VarType symtab_lookup(const char *name)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(table[i].name, name) == 0)
            return table[i].type;
    }
    return TYPE_INT; // default – we'll improve later
}

const char *ctype_string(VarType type)
{
    switch (type)
    {
    case TYPE_INT:
        return "int";
    case TYPE_FLOAT:
        return "double";
    case TYPE_STRING:
        return "char *";
    case TYPE_CHAR:
        return "char";
    case TYPE_BOOL:
        return "int";
    default:
        return "int";
    }
}

const char *ctype_spec_string(VarType type)
{
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
        return "%d";
    default:
        return "%d";
    }
}