#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Helper: check if a word is a keyword
static QTokenType check_keyword(const char *word)
{
    if (strcmp(word, "print") == 0)
        return QTOKEN_PRINT;
    if (strcmp(word, "true") == 0)
        return QTOKEN_TRUE;
    if (strcmp(word, "false") == 0)
        return QTOKEN_FALSE;
    if (strcmp(word, "let") == 0)
        return QTOKEN_LET;
    if (strcmp(word, "int") == 0)
        return QTOKEN_TYPE_INT;
    if (strcmp(word, "float") == 0)
        return QTOKEN_TYPE_FLOAT;
    if (strcmp(word, "string") == 0)
        return QTOKEN_TYPE_STRING;
    if (strcmp(word, "char") == 0)
        return QTOKEN_TYPE_CHAR;
    if (strcmp(word, "bool") == 0)
        return QTOKEN_TYPE_BOOL;
    if (strcmp(word, "if") == 0)
        return QTOKEN_IF;
    if (strcmp(word, "elif") == 0)
        return QTOKEN_ELIF;
    if (strcmp(word, "else") == 0)
        return QTOKEN_ELSE;
    return QTOKEN_UNKNOWN; // not a keyword, to indentify err
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

// Reads a single character literal (including escape sequences).
// src: source code, pos: pointer to current position (should be at the opening quote).
// Returns the decoded character, or -1 on error (unterminated, empty, etc.).
// Advances *pos past the closing quote.
static int read_char(const char *src, int *pos)
{
    (*pos)++; // skip opening quote
    if (src[*pos] == '\0')
        return -1; // unterminated

    int ch = 0;
    if (src[*pos] == '\\')
    {
        // escape sequence
        (*pos)++; // skip backslash
        if (src[*pos] == '\0')
            return -1;
        switch (src[*pos])
        {
        case 'n':
            ch = '\n';
            break;
        case 't':
            ch = '\t';
            break;
        case 'r':
            ch = '\r';
            break;
        case '\\':
            ch = '\\';
            break;
        case '\'':
            ch = '\'';
            break;
        case '0':
            ch = '\0';
            break;
        case 'a':
            ch = '\a';
            break;
        case 'b':
            ch = '\b';
            break;
        case 'f':
            ch = '\f';
            break;
        case 'v':
            ch = '\v';
            break;
        default:
            ch = src[*pos];
            break; // unrecognized escape, keep the char
        }
    }
    else
    {
        ch = src[*pos];
    }
    (*pos)++;
    if (src[*pos] != '\'')
    {
        // missing closing quote
        return -1;
    }
    (*pos)++; // skip closing quote
    return ch;
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
        Token t;
        t.value = 0;
        if (type == QTOKEN_UNKNOWN)
        {
            // It's an identifier (variable name)
            t.type = QTOKEN_IDENTIFIER;
            t.str = word; // we take ownership of the malloc'd string
        }
        else
        {
            t.type = type;
            t.str = NULL;
            free(word);
        }
        return t;
    }

    // Digits – integer literal
    // --- Number (integer or float) ---
    if (isdigit(current) || (current == '.' && isdigit(source[*pos + 1])))
    {
        int start = *pos;
        bool has_dot = false;
        bool has_exp = false;
        bool valid = true;

        // State 0: integer part (digits, optional leading dot)
        while (isdigit(source[*pos]) || (source[*pos] == '.' && !has_dot && !has_exp))
        {
            if (source[*pos] == '.')
                has_dot = true;
            (*pos)++;
        }

        // State 1: exponent part (optional)
        if (source[*pos] == 'e' || source[*pos] == 'E')
        {
            has_exp = true;
            (*pos)++;
            // optional sign
            if (source[*pos] == '+' || source[*pos] == '-')
                (*pos)++;
            // must have at least one digit
            if (!isdigit(source[*pos]))
            {
                valid = false; // exponent without digits
            }
            while (isdigit(source[*pos]))
                (*pos)++;
        }

        int length = *pos - start;
        char *num_str = malloc(length + 1);
        strncpy(num_str, &source[start], length);
        num_str[length] = '\0';

        Token t;
        if (!valid)
        {
            t.type = QTOKEN_UNKNOWN;
            t.value = 0;
            t.floatValue = 0.0;
        }
        else
        {
            if (has_dot || has_exp)
            {
                char *endptr;
                t.type = QTOKEN_FLOAT;
                t.floatValue = strtod(num_str, &endptr);
                if (endptr == num_str)
                {
                    t.type = QTOKEN_UNKNOWN;
                    t.floatValue = 0.0;
                }
            }
            else
            {
                t.type = QTOKEN_INTEGER;
                t.value = atoi(num_str);
                t.floatValue = 0.0;
            }
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

    // Character literal
    if (current == '\'')
    {
        int ch = read_char(source, pos);
        if (ch == -1)
        {
            Token t = {.type = QTOKEN_UNKNOWN, .value = 0, .str = NULL};
            return t;
        }
        Token t = {.type = QTOKEN_CHAR, .value = ch, .str = NULL};
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
    case ',':
        (*pos)++;
        {
            Token t = {.type = QTOKEN_COMMA, .value = 0, .str = NULL};
            return t;
        }
    case ':':
        (*pos)++;
        {
            Token t = {.type = QTOKEN_COLON, .value = 0, .str = NULL};
            return t;
        }
    case '=':
        (*pos)++;
        if (source[*pos] == '=')
        {
            (*pos)++;
            Token t = {.type = QTOKEN_EQ_EQ, .value = 0, .str = NULL};
            return t;
        }
        else
        {
            Token t = {.type = QTOKEN_EQUALS, .value = 0, .str = NULL};
            return t;
        }
    case '+':
        (*pos)++;
        {
            Token t = {.type = QTOKEN_PLUS, .value = 0, .str = NULL};
            return t;
        }
    case '-':
        (*pos)++;
        {
            Token t = {.type = QTOKEN_MINUS, .value = 0, .str = NULL};
            return t;
        }
    case '*':
        (*pos)++;
        if (source[*pos] == '*')
        {
            (*pos)++;
            {
                Token t = {.type = QTOKEN_POWER, .value = 0, .str = NULL};
                return t;
            }
        }
        else
        {
            {
                Token t = {.type = QTOKEN_STAR, .value = 0, .str = NULL};
                return t;
            }
        }
    case '/':
        (*pos)++;
        if (source[*pos] == '/')
        {
            (*pos)++;
            {
                Token t = {.type = QTOKEN_FLOOR_DIV, .value = 0, .str = NULL};
                return t;
            }
        }
        else
        {
            {
                Token t = {.type = QTOKEN_SLASH, .value = 0, .str = NULL};
                return t;
            }
        }
    case '%':
        (*pos)++;
        {
            Token t = {.type = QTOKEN_PERCENT, .value = 0, .str = NULL};
            return t;
        }
    case '<':
        (*pos)++;
        if (source[*pos] == '=')
        {
            (*pos)++;
            Token t = {.type = QTOKEN_LT_EQ, .value = 0, .str = NULL};
            return t;
        }
        else
        {
            Token t = {.type = QTOKEN_LT, .value = 0, .str = NULL};
            return t;
        }
    case '>':
        (*pos)++;
        if (source[*pos] == '=')
        {
            (*pos)++;
            Token t = {.type = QTOKEN_GT_EQ, .value = 0, .str = NULL};
            return t;
        }
        else
        {
            Token t = {.type = QTOKEN_GT, .value = 0, .str = NULL};
            return t;
        }
    case '!':
        (*pos)++;
        if (source[*pos] == '=')
        {
            (*pos)++;
            Token t = {.type = QTOKEN_NOT_EQ, .value = 0, .str = NULL};
            return t;
        }
        else
        {
            Token t = {.type = QTOKEN_NOT, .value = 0, .str = NULL};
            return t;
        }
    case '{':
        (*pos)++;
        {
            Token t = {.type = QTOKEN_LBRACE, .value = 0, .str = NULL};
            return t;
        }
    case '}':
        (*pos)++;
        {
            Token t = {.type = QTOKEN_RBRACE, .value = 0, .str = NULL};
            return t;
        }
    case '&':
        (*pos)++;
        if (source[*pos] == '&')
        {
            (*pos)++;
            Token t = {.type = QTOKEN_AND, .value = 0, .str = NULL};
            return t;
        }
        else
        {
            // bitwise & not yet supported
            Token t = {.type = QTOKEN_UNKNOWN, .value = 0, .str = NULL};
            return t;
        }
    case '|':
        (*pos)++;
        if (source[*pos] == '|')
        {
            (*pos)++;
            Token t = {.type = QTOKEN_OR, .value = 0, .str = NULL};
            return t;
        }
        else
        {
            // bitwise | not yet supported
            Token t = {.type = QTOKEN_UNKNOWN, .value = 0, .str = NULL};
            return t;
        }
    }

    // Unknown char
    Token t = {.type = QTOKEN_UNKNOWN, .value = 0, .str = NULL};
    (*pos)++;
    return t;
}