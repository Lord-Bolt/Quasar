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

// Processes escape sequences in a string literal.
// src: the source code starting at the opening quote.
// pos: current position (should be just after the opening quote).
// Returns a newly allocated string with escapes translated, and advances *pos past the closing quote.
// On error (unterminated string), returns NULL.
static char *read_string(const char *src, int *pos)
{
    size_t cap = 64;
    size_t len = 0;
    char *buffer = malloc(cap);
    if (!buffer)
        return NULL;

    while (src[*pos] != '"')
    {
        if (src[*pos] == '\0')
        {
            free(buffer); // Unterminated string
            return NULL;
        }

        if (src[*pos] == '\\')
        {
            (*pos)++; // skip backslash
            if (src[*pos] == '\0')
            {
                free(buffer);
                return NULL;
            }
            switch (src[*pos])
            {
            case 'n':
                buffer[len++] = '\n';
                break;
            case 't':
                buffer[len++] = '\t';
                break;
            case 'r':
                buffer[len++] = '\r';
                break; // carriage return
            case '\\':
                buffer[len++] = '\\';
                break;
            case '"':
                buffer[len++] = '\"';
                break;
            case '0':
                buffer[len++] = '\0';
                break; // null character (use carefully)
            case 'a':
                buffer[len++] = '\a';
                break; // alert (bell)
            case 'b':
                buffer[len++] = '\b';
                break; // backspace
            case 'f':
                buffer[len++] = '\f';
                break; // form feed
            case 'v':
                buffer[len++] = '\v';
                break; // vertical tab
            default:
                buffer[len++] = '\\';
                buffer[len++] = src[*pos];
                break;
            }
        }
        else
        {
            buffer[len++] = src[*pos];
        }
        (*pos)++;
        if (len >= cap - 1)
        {
            cap *= 2;
            char *newbuf = realloc(buffer, cap);
            if (!newbuf)
            {
                free(buffer);
                return NULL;
            }
            buffer = newbuf;
        }
    }
    buffer[len] = '\0';
    (*pos)++;
    return buffer;
}

Token get_next_token(const char *source, int *pos)
{
    // Skip Whitespace
    while (source[*pos] == ' ' || source[*pos] == '\t' || source[*pos] == '\n' || source[*pos] == '\r')
    {
        (*pos)++;
    }
    char current = source[*pos];

    // EOF
    if (current == '\0')
    {
        Token t = {QTOKEN_EOF, 0, NULL};
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

        Token t = {type, 0, NULL};
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

        Token t = {QTOKEN_INTEGER, value, NULL};
        return t;
    }

    // In get_next_token, after the digit block and before the single‑character switch:
    if (current == '"')
    {
        (*pos)++; // skip opening quote
        char *str = read_string(source, pos);
        if (str == NULL)
        {
            Token t = {QTOKEN_UNKNOWN, 0, NULL};
            return t;
        }
        Token t = {QTOKEN_STRING, 0, str};
        return t;
    }

    // Single Char Tokens
    switch (current)
    {
    case '(':
        (*pos)++;
        {
            Token t = {QTOKEN_LPAREN, 0, NULL};
            return t;
        }
    case ')':
        (*pos)++;
        {
            Token t = {QTOKEN_RPAREN, 0, NULL};
            return t;
        }
    case ';':
        (*pos)++;
        {
            Token t = {QTOKEN_SEMICOLON, 0, NULL};
            return t;
        }
    }

    // Unknown char
    Token t = {QTOKEN_UNKNOWN, 0, NULL};
    (*pos)++;
    return t;
}