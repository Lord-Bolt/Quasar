#ifndef LEXER_H
#define LEXER_H

typedef enum
{
    QTOKEN_PRINT,
    QTOKEN_LPAREN,
    QTOKEN_RPAREN,
    QTOKEN_INTEGER,
    QTOKEN_SEMICOLON,
    QTOKEN_EOF,
    QTOKEN_UNKNOWN
} QTokenType;

typedef struct
{
    QTokenType type;
    int value;
} Token;

Token get_next_token(const char *source, int *pos);

#endif