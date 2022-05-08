#pragma once
#include "standard.h"
#include "memory.h"

typedef enum {

	// primary tokens
	TOKEN_INT = 256,
	TOKEN_IDENTIFIER,

	// two character tokens
	TOKEN_EQ,
	TOKEN_NE,
	TOKEN_LE,
	TOKEN_GE,

	// keywords
	TOKEN_RETURN,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_FOR,
	TOKEN_WHILE,

	// context-based tokens
	TOKEN_NEGATIVE,
	TOKEN_POSITIVE,
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
