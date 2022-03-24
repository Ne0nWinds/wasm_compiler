#include "token.h"
#include "standard.h"
#include "memory.h"

#define is_digit(c) (c >= '0' && c <= '9')
#define is_whitespace(c) (c == ' ' || c == '\t' || c == '\r' || c == '\n')

u32 int_str_len(char *text) {
	char *text2 = text;
	while (*text2 && is_digit(*text2)) text2 += 1;
	return text2 - text;
}

int int_from_str(char *text, u32 length) {
	text += length - 1;
	int value = 0;

	while (length) {
		value *= 10;
		value += *text - '0';
		text -= 1;
		length -= 1;
	}

	return value;
}

List token_list;

List tokenize_text(char *text, u32 length) {
	token_list.length = 0;
	token_list.start = bump_get();

	for (int i = 0; i < length; ++i) {
		token t = {0};
		char c = text[i];

		if (is_whitespace(c)) continue;

		if (is_digit(c)) {
			t.type = TOKEN_INT;
			u32 int_len = int_str_len(text + i);
			t.value = int_from_str(text + i, int_len);
		} else if (c == '+') {
			t.type = TOKEN_PLUS;
		} else if (c == '-') {
			t.type = TOKEN_MINUS;
		}

		list_add(token_list, t);
	}
	bump_move(token_list.length * sizeof(token));

	return token_list;
}
