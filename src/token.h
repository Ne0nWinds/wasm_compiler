#pragma once
#include "standard.h"
#include "memory.h"

typedef enum {
	TOKEN_INT = 1,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_OPEN_PARENTHESIS,
	TOKEN_CLOSED_PARENTHESIS,
	TOKEN_NEGATIVE,
	TOKEN_POSITIVE,
	TOKEN_EQ,
	TOKEN_NE,
	TOKEN_LT,
	TOKEN_LE,
	TOKEN_GT,
	TOKEN_GE,
	TOKEN_SEMICOLON,
	TOKEN_IDENTIFIER,
	TOKEN_KEYWORD,
	TOKEN_ASSIGN,
	TOKEN_RETURN,
	TOKEN_OPEN_BRACKET,
	TOKEN_CLOSED_BRACKET,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_FOR,

} token_type;

typedef struct token token;
struct token {
	token_type type;
	union {
		struct {
			char *name;
			u32 length;
		} identifier;
		int value;
	};
};

List tokenize_text(char *text, u32 length, bool *unexpected_token);
