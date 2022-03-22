#pragma once
#include "standard.h"
#include "memory.h"

typedef enum {
	TOKEN_INT,
	TOKEN_PLUS,
	TOKEN_MINUS,
} token;

List tokenize_text(char *text, u32 length);
