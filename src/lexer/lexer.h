#ifndef LEXER_H
#define LEXER_H

typedef enum
{
    QTOKEN_PRINT,
    QTOKEN_LPAREN,
    QTOKEN_RPAREN,
    QTOKEN_LBRACE,
    QTOKEN_RBRACE,
    QTOKEN_INTEGER,
    QTOKEN_FLOAT,
    QTOKEN_STRING,
    QTOKEN_CHAR,
    QTOKEN_IDENTIFIER,
    QTOKEN_TYPE_INT,
    QTOKEN_TYPE_FLOAT,
    QTOKEN_TYPE_STRING,
    QTOKEN_TYPE_CHAR,
    QTOKEN_TYPE_BOOL,
    QTOKEN_TRUE,
    QTOKEN_FALSE,
    QTOKEN_SEMICOLON,
    QTOKEN_COMMA,
    QTOKEN_LET,
    QTOKEN_COLON,
    QTOKEN_EQUALS,
    QTOKEN_EOF,
    QTOKEN_PLUS,      // +
    QTOKEN_MINUS,     // -
    QTOKEN_STAR,      // *
    QTOKEN_SLASH,     // /
    QTOKEN_PERCENT,   // %
    QTOKEN_POWER,     // **
    QTOKEN_FLOOR_DIV, // //
    QTOKEN_LT,        // <
    QTOKEN_GT,        // >
    QTOKEN_LT_EQ,     // <=
    QTOKEN_GT_EQ,     // >=
    QTOKEN_EQ_EQ,     // ==
    QTOKEN_NOT_EQ,    // !=
    QTOKEN_AND,       // &&
    QTOKEN_OR,        // ||
    QTOKEN_NOT,       // ! (unary not)
    QTOKEN_UNKNOWN
} QTokenType;

typedef struct
{
    QTokenType type;
    int value;
    double floatValue;
    char *str;
} Token;

Token get_next_token(const char *source, int *pos);

#endif