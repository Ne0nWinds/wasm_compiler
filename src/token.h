#pragma once
#include "standard.h"
#include "memory.h"

typedef enum {
	TOKEN_INT = 1,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_STAR,
	TOKEN_FSLASH,
} token_type;

typedef struct token token;
struct token {
	token_type type;
	int value;
};

List tokenize_text(char *text, u32 length);
