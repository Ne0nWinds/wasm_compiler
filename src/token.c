#include "token.h"
#include "standard.h"
#include "memory.h"

#define is_digit(c) (c >= '0' && c <= '9')
#define is_whitespace(c) (c == ' ' || c == '\t' || c == '\r' || c == '\n')
#define is_alpha(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
#define is_alpha_numeric(c) (is_digit(c) || is_alpha(c))

u32 int_str_len(char *text) {
	char *text2 = text;
	while (*text2 && is_digit(*text2)) text2 += 1;
	return text2 - text;
}

int int_from_str(char *text, u32 length) {
	int value = 0;

	for (int i = 0; i < length; ++i) {
		value *= 10;
		value += *text - '0';
		text += 1;
	}

	return value;
}

static List token_list = {0};

List tokenize_text(char *text, u32 length, bool *unexpected_token) {
	*unexpected_token = false;
	token_list.length = 0;
	token_list.start = bump_get();

	int i = 0;
	for (; i < length; ++i) {
		token t = {0};
		char *c = text + i;

		if (is_whitespace(*c)) continue;

		if (is_digit(*c)) {
			t.type = TOKEN_INT;
			u32 int_len = int_str_len(text + i);
			t.value = int_from_str(text + i, int_len);
			i += int_len - 1;
			list_add(token_list, t);
			continue;
		}

		if (is_alpha(*c)) {
			t.type = TOKEN_IDENTIFIER;
			t.identifier.name = c;
			t.identifier.length = 1;
			list_add(token_list, t);
			continue;
		}

		token prev_token = {0};
		if (i > 0)
			prev_token = list_get(token_list, token, token_list.length - 1);

		switch (*c) {
			case '+': {
				t.type = TOKEN_PLUS;
				if (prev_token.type != TOKEN_INT && prev_token.type != TOKEN_IDENTIFIER) {
					t.type = TOKEN_POSITIVE;
				}
			} break;
			case '-': {
				t.type = TOKEN_MINUS;
				if (prev_token.type != TOKEN_INT && prev_token.type != TOKEN_IDENTIFIER) {
					t.type = TOKEN_NEGATIVE;
				}
			} break;
			case '*': {
				t.type = TOKEN_MUL;
			} break;
			case '/': {
				t.type = TOKEN_DIV;
			} break;
			case '(': {
				t.type = TOKEN_OPEN_PARENTHESIS;
			} break;
			case ')': {
				t.type = TOKEN_CLOSED_PARENTHESIS;
			} break;
			case '<': {
				t.type = TOKEN_LT;
				if (*(c + 1) == '=') {
					t.type = TOKEN_LE;
					i += 1;
				}
			} break;
			case '>': {
				t.type = TOKEN_GT;
				if (*(c + 1) == '=') {
					t.type = TOKEN_GE;
					i += 1;
				}
			} break;
			case '!': {
				// t.type == NOT operator
				if (*(c + 1) == '=') {
					t.type = TOKEN_NE;
					i += 1;
				}
			} break;
			case '=': {
				t.type = TOKEN_ASSIGN;
				if (*(c + 1) == '=') {
					t.type = TOKEN_EQ;
					i += 1;
				}
			} break;
			case ';': {
				t.type = TOKEN_SEMICOLON;
			} break;
			default: {
				*unexpected_token = true;
				goto end;
			}
		}

		list_add(token_list, t);
}

end:
	bump_move(token_list.length * sizeof(token));
	return token_list;
}
