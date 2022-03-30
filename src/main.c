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

	typedef struct operator operator;
	struct operator {
		u8 paren_level;
		u8 precedence;
		u8 op;
	};

	u32 current_paren_level = 0;

	for (u32 i = 0; i < token_list.length; ++i) {

		token t = list_get(token_list, token, i);

		if (t.type == TOKEN_INT) {
			*c++ = WASM_I32_CONST;
			*c++ = list_get(token_list, token, i).value;
			continue;
		}

		if (t.type == TOKEN_OPEN_PARENTHESIS) {
			// change precedence
			current_paren_level += 1;
			continue;
		}

		if (t.type == TOKEN_CLOSED_PARENTHESIS) {
			// unwind stack to open paren

			for (int i = operators.length - 1; i >= 0; --i) {
				operator op = list_get(operators, operator, i);
				if (op.paren_level != current_paren_level) break;
				operators.length -= 1;
				*c++ = op.op;
			}

			current_paren_level -= 1;
			continue;
		}

		operator op = {0};
		op.paren_level = current_paren_level;

		switch (t.type) {
			case TOKEN_PLUS: {
				op.op = WASM_I32_ADD;
				op.precedence = 1;
			} break;
			case TOKEN_MINUS: {
				op.op = WASM_I32_SUB;
				op.precedence = 1;
			} break;
			case TOKEN_MUL: {
				op.op = WASM_I32_MUL;
				op.precedence = 2;
			} break;
			case TOKEN_DIV: {
				op.op = WASM_I32_DIV_S;
				op.precedence = 2;
			} break;
		}

		operator prev_op = list_get(operators, operator, operators.length - 1);
		u32 op_precedence = op.paren_level * 2 + op.precedence;
		u32 prev_op_precedence = prev_op.paren_level * 2 + prev_op.precedence;

		if (!operators.length) {
			list_push(operators, op);
			continue;
		}

		if (op_precedence > prev_op_precedence) {
			list_push(operators, op);
			continue;
		}

		if (op_precedence <= prev_op_precedence) {
			*c++ = prev_op.op;
			operators.length -= 1;
			list_push(operators, op);
			continue;
		}
	}

	for (int i = operators.length - 1; i >= 0; --i) {
		operator op = list_get(operators, operator, i);
		*c++ = op.op;
	}

	*c++ = 0xB;

	*code_section_length += c - code_start;
	*code_section_length2 += c - code_start;

	return c - wasm_binary;
}

__attribute__((export_name("get_wasm_binary")))
unsigned char *get_wasm_binary() {
	return wasm_binary;
}
