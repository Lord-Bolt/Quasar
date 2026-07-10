#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// Helper: check if a word is a keyword (for now, just "print").
static QTokenType check_keyword(const char *word)
{
    if (strcmp(word, "print") == 0)
        return QTOKEN_PRINT;
    // In future: else if (strcmp(word, "let") == 0) return TOKEN_LET; etc.
    return QTOKEN_UNKNOWN; // not a keyword, maybe an identifier later
}

Token get_next_token(const char *source, int *pos)
{
    // Skip Whitespace
    while (source[*pos] == ' ' || source[*pos] == '\t' || source[*pos] == '\n')
    {
        (*pos)++;
    }
    char current = source[*pos];

    // EOF
    if (current == '\0')
    {
        Token t = {QTOKEN_EOF, 0};
        return t;
    }

    // Letters – could be a keyword or identifier
    if (isalpha(current))
    {
        int start = *pos;
        while (isalnum(source[*pos]) || source[*pos] == '_')
        {
            (*pos)++;
        }
        int length = *pos - start;
        char *word = (char *)malloc(length + 1);
        strncpy(word, &source[start], length);
        word[length] = '\0';

        QTokenType type = check_keyword(word);
        free(word);

        Token t = {type, 0};
        if (type == QTOKEN_UNKNOWN)
        {
            // Could be an identifier in the future, but for now it's unknown.
        }
        return t;
    }

    // Digits – integer literal
    if (isdigit(current))
    {
        int start = *pos;
        while (isdigit(source[*pos]))
        {
            (*pos)++;
        }
        int length = *pos - start;
        char *num_str = (char *)malloc(length + 1);
        strncpy(num_str, &source[start], length);
        num_str[length] = '\0';
        int value = atoi(num_str);
        free(num_str);

        Token t = {QTOKEN_INTEGER, value};
        return t;
    }

    // Single Char Tokens
    switch (current)
    {
    case '(':
        (*pos)++;
        {
            Token t = {QTOKEN_LPAREN, 0};
            return t;
        }
    case ')':
        (*pos)++;
        {
            Token t = {QTOKEN_RPAREN, 0};
            return t;
        }
    case ';':
        (*pos)++;
        {
            Token t = {QTOKEN_SEMICOLON, 0};
            return t;
        }
    }

    // Unknown char
    Token t = {QTOKEN_UNKNOWN, 0};
    (*pos)++;
    return t;
}