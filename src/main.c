#include "standard.h"
#include "wasm_defs.h"
#include "memory.h"
#include "token.h"

static unsigned char *wasm_binary;
static u32 wasm_length;

static u8 *wasm_header(u8 *binary) {

	// WASM Magic Number
	binary[0] = 0;
	binary[1] = 'a';
	binary[2] = 's';
	binary[3] = 'm';

	// WASM version
	binary[4] = 1;
	binary[5] = 0;
	binary[6] = 0;
	binary[7] = 0;

	return wasm_binary + 8;;
}

static u8 *create_main_function(u8 *binary) {

	binary[0] = WASM_SECTION_TYPE;
	binary[1] = 0x5;

	binary[2] = 0x1;
	binary[3] = WASM_FUNCTION_TYPE;
	binary[4] = 0x0; // parameters list
	binary[5] = 0x1; // return value list
	binary[6] = WASM_NUMTYPE_I32;
	binary += 7;


	binary[0] = WASM_SECTION_FUNC;
	binary[1] = 0x2;

	binary[2] = 0x1;
	binary[3] = 0x0;
	binary += 4;


	binary[0] = WASM_SECTION_EXPORT;
	binary[1] = 0x8;

	binary[2] = 0x1;
	binary[3] = 0x4;
	binary[4] = 'm';
	binary[5] = 'a';
	binary[6] = 'i';
	binary[7] = 'n';
	binary[8] = 0x0;
	binary[9] = 0x0;
	binary += 10;

	return binary;
}

__attribute__((export_name("compile")))
int compile(char *text, u32 length) {

	wasm_binary = bump_alloc(1024 * 32);

	u8 *c = wasm_binary;
	c = wasm_header(c);
	c = create_main_function(c);

	List token_list = tokenize_text(text, length);

	c[0] = WASM_SECTION_CODE;
	c[1] = 0x3;
	c[2] = 0x1; // vec(code)
	c[3] = 0x1;
	c[4] = 0x0;

	u8 *code_section_length = c + 1;
	u8 *code_section_length2 = c + 3;

	c += 5;

	u8 *code_start = c;

	List operators = {
		.length = 0,
		.start = bump_get()
	};

	*c++ = WASM_I32_CONST;
	*c++ = list_get(token_list, token, 0).value;
	for (int i = 1; i < token_list.length; i += 2) {
		token op_token = list_get(token_list, token, i);
		token number_token = list_get(token_list, token, i + 1);

		u8 op = 0;
		u8 op_precedence = 0;
		u8 prev_op = list_get(operators, u8, operators.length - 1);
		u8 prev_op_precedence = 0;

		switch (op_token.type) {
			case TOKEN_PLUS: {
				op = WASM_I32_ADD;
				op_precedence = 1;
			} break;
			case TOKEN_MINUS: {
				op = WASM_I32_SUB;
				op_precedence = 1;
			} break;
			case TOKEN_MUL: {
				op = WASM_I32_MUL;
				op_precedence = 2;
			} break;
			case TOKEN_DIV: {
				op = WASM_I32_DIV_S;
				op_precedence = 2;
			} break;
		}

		switch (prev_op) {
			case WASM_I32_ADD:
			case WASM_I32_SUB: {
				prev_op_precedence = 1;
			} break;
			case WASM_I32_DIV_S:
			case WASM_I32_MUL: {
				prev_op_precedence = 2;
			} break;
		}

		if (!operators.length) {
			*c++ = WASM_I32_CONST;
			*c++ = number_token.value;
			list_push(operators, op);
			continue;
		}

		if (prev_op_precedence >= op_precedence) {
			*c++ = prev_op;
			*c++ = WASM_I32_CONST;
			*c++ = number_token.value;
			operators.length -= 1;
			list_push(operators, op);
			continue;
		}

		if (prev_op_precedence < op_precedence) {
			*c++ = WASM_I32_CONST;
			*c++ = number_token.value;
			list_push(operators, op);
			continue;
		}
	}

	for (int i = operators.length - 1; i >= 0; --i) {
		u8 op = list_get(operators, u8, i);
		*c++ = op;
	}

	/* for (int i = 1; i < token_list.length; i += 2) {
		token t = ((token *)token_list.start)[i + 1];
		*c++ = WASM_I32_CONST;
		*c++ = t.value;

		token t2 = ((token *)token_list.start)[i];
		u8 *op = c;
		c += 1;
		switch (t2.type) {
			case TOKEN_PLUS: {
				*op = WASM_I32_ADD;
			} break;
			case TOKEN_MINUS: {
				*op = WASM_I32_SUB;
			} break;
			case TOKEN_STAR: {
				*op = WASM_I32_MUL;
			} break;
			case TOKEN_FSLASH: {
				*op = WASM_I32_DIV_S;
			} break;
		}
	} */

	*c++ = 0xB;

	*code_section_length += c - code_start;
	*code_section_length2 += c - code_start;

	return c - wasm_binary;
}

__attribute__((export_name("get_wasm_binary")))
unsigned char *get_wasm_binary() {
	return wasm_binary;
}
