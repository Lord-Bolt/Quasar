#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Helper: check if a word is a keyword (for now, just "print").
static QTokenType check_keyword(const char *word)
{
    if (strcmp(word, "print") == 0)
        return QTOKEN_PRINT;
    // In future: else if (strcmp(word, "let") == 0) return TOKEN_LET; etc.
    return QTOKEN_UNKNOWN; // not a keyword, maybe an identifier later
}

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
        Token t = {.type = QTOKEN_EOF, .value = 0, .str = NULL};
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

        Token t = {.type = type, .value = 0, .str = NULL};
        if (type == QTOKEN_UNKNOWN)
        {
            // Could be an identifier in the future, but for now it's unknown.
        }
        return t;
    }

    // Digits – integer literal
    // --- Number (integer or float) ---
    if (isdigit(current) || (current == '.' && isdigit(source[*pos + 1])))
    {
        int start = *pos;
        // Scan forward while the character belongs to the number
        while (isdigit(source[*pos]) || source[*pos] == '.' ||
               source[*pos] == 'e' || source[*pos] == 'E' ||
               ((source[*pos] == '+' || source[*pos] == '-') &&
                (source[*pos - 1] == 'e' || source[*pos - 1] == 'E')))
        {
            (*pos)++;
        }
        int length = *pos - start;
        char *num_str = (char *)malloc(length + 1);
        strncpy(num_str, &source[start], length);
        num_str[length] = '\0';

        // Determine if it's a float (contains '.' or 'e'/'E')
        bool is_float = false;
        for (int i = 0; i < length; i++)
        {
            if (num_str[i] == '.' || num_str[i] == 'e' || num_str[i] == 'E')
            {
                is_float = true;
                break;
            }
        }

        Token t;
        if (is_float)
        {
            char *endptr;
            t.type = QTOKEN_FLOAT;
            t.floatValue = strtod(num_str, &endptr);
            if (endptr == num_str)
            {
                // conversion failed, treat as unknown
                t.type = QTOKEN_UNKNOWN;
                t.floatValue = 0.0;
            }
        }
        else
        {
            t.type = QTOKEN_INTEGER;
            t.value = atoi(num_str);
        }
        t.str = NULL;
        free(num_str);
        return t;
    }

    // In get_next_token, after the digit block and before the single‑character switch:
    if (current == '"')
    {
        (*pos)++; // skip opening quote
        char *str = read_string(source, pos);
        if (str == NULL)
        {
            Token t = {.type = QTOKEN_UNKNOWN, .value = 0, .str = NULL};
            return t;
        }
        Token t = {.type = QTOKEN_STRING, .value = 0, .str = str};
        return t;
    }

    // Single Char Tokens
    switch (current)
    {
    case '(':
        (*pos)++;
        {
            Token t = {.type = QTOKEN_LPAREN, .value = 0, .str = NULL};
            return t;
        }
    case ')':
        (*pos)++;
        {
            Token t = {.type = QTOKEN_RPAREN, .value = 0, .str = NULL};
            return t;
        }
    case ';':
        (*pos)++;
        {
            Token t = {.type = QTOKEN_SEMICOLON, .value = 0, .str = NULL};
            return t;
        }
    }

    // Unknown char
    Token t = {.type = QTOKEN_UNKNOWN, .value = 0, .str = NULL};
    (*pos)++;
    return t;
}