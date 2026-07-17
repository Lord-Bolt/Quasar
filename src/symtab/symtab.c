#include "symtab.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_VARS 256
#define MAX_SCOPES 64

typedef struct
{
    char name[64];
    VarType type;
} Symbol;

typedef struct
{
    Symbol symbols[MAX_VARS];
    int count;
} Scope;

static Scope scopes[MAX_SCOPES];
static int scope_top = -1;

void symtab_push_scope(void)
{
    if (scope_top < MAX_SCOPES - 1)
    {
        scope_top++;
        scopes[scope_top].count = 0;
    }
    else
    {
        fprintf(stderr, "Fatal: scope stack overflow\n");
        exit(1);
    }
}

void symtab_pop_scope(void)
{
    if (scope_top >= 0)
    {
        scope_top--;
    }
}

void symtab_add(const char *name, VarType type)
{
    if (scope_top < 0)
    {
        symtab_push_scope(); // safety: create a global scope if none exists
    }
    Scope *cur = &scopes[scope_top];
    if (cur->count < MAX_VARS)
    {
        strncpy(cur->symbols[cur->count].name, name, 63);
        cur->symbols[cur->count].name[63] = '\0';
        cur->symbols[cur->count].type = type;
        cur->count++;
    }
    else
    {
        fprintf(stderr, "Error: too many variables in this scope\n");
    }
}

VarType symtab_lookup(const char *name)
{
    for (int s = scope_top; s >= 0; s--)
    {
        for (int i = 0; i < scopes[s].count; i++)
        {
            if (strcmp(scopes[s].symbols[i].name, name) == 0)
                return scopes[s].symbols[i].type;
        }
    }
    return TYPE_INT; // add a sentinel later
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
        return "bool";
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