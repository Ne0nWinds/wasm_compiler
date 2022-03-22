#include "token.h"
#include "standard.h"
#include "memory.h"

#define is_digit(c) (c >= '0' && c <= '9')
#define is_whitespace(c) (c == ' ' || c == '\t' || c == '\r' || c == '\n')

List token_list;

List tokenize_text(char *text, u32 length) {
	token_list.length = 0;
	token_list.start = bump_get();

	for (int i = 0; i < length; ++i) {
		token t = 0;
		char c = text[i];

		if (is_whitespace(c)) continue;

		if (is_digit(c)) {
			t = TOKEN_INT;
		} else if (c == '+') {
			t = TOKEN_PLUS;
		} else if (c == '-') {
			t = TOKEN_MINUS;
		}

		list_add(token_list, t);
	}
	bump_move(token_list.length * sizeof(token));

	return token_list;
}
