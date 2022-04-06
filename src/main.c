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

	binary[0] = WASM_SECTION_MEMORY;
	binary[1] = 3;
	binary[2] = 1;
	binary[3] = 0;
	binary[4] = 1;
	binary += 5;

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

u32 depth = 0;

u8 *token_to_op(u32 token_type, u8 *binary) {
	u32 byte_length = 1;
	depth -= 1;
	switch (token_type) {
		case TOKEN_PLUS: {
			*binary = WASM_I32_ADD;
		} break;
		case TOKEN_MINUS: {
			*binary = WASM_I32_SUB;
		} break;
		case TOKEN_MUL: {
			*binary = WASM_I32_MUL;
		} break;
		case TOKEN_DIV: {
			*binary = WASM_I32_DIV_S;
		} break;
		case TOKEN_NEGATIVE: {
			*binary = WASM_I32_CONST;
			EncodeLEB128(binary + 1, -1, byte_length);
			binary[byte_length++] = WASM_I32_MUL;
		} break;
		case TOKEN_LT: {
			*binary = WASM_I32_LT_S;
		} break;
		case TOKEN_LE: {
			*binary = WASM_I32_LE_S;
		} break;
		case TOKEN_GT: {
			*binary = WASM_I32_GT_S;
		} break;
		case TOKEN_GE: {
			*binary = WASM_I32_GE_S;
		} break;
		case TOKEN_ASSIGN: {
			byte_length = 6;
			*binary = WASM_I32_STORE;
			binary[1] = 2;
			binary[2] = 0;
			binary[3] = WASM_I32_LOAD;
			binary[4] = 2;
			binary[5] = 0;
			depth += 1;
		} break;
	}
	return binary + byte_length;
}

static List token_list = {0};
static token *current_token = 0;
static bool error_occurred = 0;

u8 *expr(u8 *c) {

	List operators = {
		.length = 0,
		.start = bump_get()
	};

	typedef struct operator operator;
	struct operator {
		u8 paren_level;
		u8 precedence;
		u8 token_type;
	};

	enum {
		PREC_COMMA = 1,
		PREC_ASSIGN,
		PREC_TERNARY,
		PREC_OR,
		PREC_AND,
		PREC_BITWISE_OR,
		PREC_BITWISE_XOR,
		PREC_BITWISE_AND,
		PREC_EQUALITY,
		PREC_RELATIONAL,
		PREC_SHIFT,
		PREC_ADD,
		PREC_MUL,
		PREC_UNARY,
		PREC_POSTFIX,
	};

	u32 current_paren_level = 0;

	for (; current_token - (token *)token_list.start < token_list.length; ++current_token) {

		token t = *current_token;

		if (t.type == TOKEN_SEMICOLON) break;

		if (t.type == TOKEN_INT) {
			*c++ = WASM_I32_CONST;
			int length = 0;
			EncodeLEB128(c, t.value, length);
			c += length;
			depth += 1;
			continue;
		}

		if (t.type == TOKEN_IDENTIFIER) {
			int offset = (t.identifier.name[0] - 'a' + 1) * 4;
			*c++ = WASM_I32_CONST;
			*c++ = offset;
			if ((current_token + 1)->type == TOKEN_ASSIGN) {
				*c++ = WASM_I32_CONST;
				*c++ = offset;
			} else {
				*c++ = WASM_I32_LOAD;
				*c++ = 2;
				*c++ = 0;
				depth += 1;
			}
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
				c = token_to_op(op.token_type, c);
			}

			current_paren_level -= 1;
			continue;
		}

		operator op = {0};
		op.token_type = t.type;
		op.paren_level = current_paren_level;

		switch (t.type) {
			case TOKEN_MINUS:
			case TOKEN_PLUS: {
				op.precedence = PREC_ADD;
			} break;
			case TOKEN_DIV:
			case TOKEN_MUL: {
				op.precedence = PREC_MUL;
			} break;
			case TOKEN_NEGATIVE: {
				op.precedence = PREC_POSTFIX;
			} break;
			case TOKEN_LT:
			case TOKEN_LE:
			case TOKEN_GT:
			case TOKEN_GE: {
				op.precedence = PREC_RELATIONAL;
			} break;
			case TOKEN_ASSIGN: {
				op.precedence = PREC_ASSIGN;
			} break;
			case TOKEN_POSITIVE: {
				continue;
			} break;
		}

		operator prev_op = list_get(operators, operator, operators.length - 1);
		u32 op_precedence = op.paren_level * PREC_POSTFIX + op.precedence;
		u32 prev_op_precedence = prev_op.paren_level * PREC_POSTFIX + prev_op.precedence;

		if (!operators.length) {
			list_push(operators, op);
			continue;
		}

		if (op_precedence > prev_op_precedence) {
			list_push(operators, op);
			continue;
		}

		if (op_precedence == PREC_ASSIGN && op_precedence == prev_op_precedence) {
			list_push(operators, op);
			continue;
		}

		if (op_precedence <= prev_op_precedence) {
			c = token_to_op(prev_op.token_type, c);
			operators.length -= 1;
			list_push(operators, op);
			continue;
		}
	}

	for (int i = operators.length - 1; i >= 0; --i) {
		operator op = list_get(operators, operator, i);
		c = token_to_op(op.token_type, c);
	}

	return c;
}

u8 *expr_stmt(u8 *c) {
	u8 *expr_code = expr(c);
	if (current_token->type == TOKEN_SEMICOLON) {
		current_token += 1;
	} else {
		error_occurred = true;
	}
	return expr_code;
}

__attribute__((export_name("compile")))
int compile(char *text, u32 length) {

	error_occurred = false;
	wasm_binary = bump_alloc(1024 * 32);

	u8 *c = wasm_binary;
	c = wasm_header(c);
	c = create_main_function(c);

	bool unexpected_token = false;
	token_list = tokenize_text(text, length, &unexpected_token);

	current_token = token_list.start;

	c[0] = WASM_SECTION_CODE;
	c[1] = 0x3;
	c[2] = 0x1; // vec(code)
	c[3] = 0x1;
	c[4] = 0x0;

	u8 *code_section_length = c + 1;
	u8 *code_section_length2 = c + 3;

	c += 5;

	u8 *code_start = c;

	depth = 0;
	while (current_token - (token *)token_list.start < token_list.length) {
		c = expr_stmt(c);
		if (depth > 0)
			*c++ = WASM_DROP;
	}

	if (depth != 0)
		c -= 1;

	*c++ = 0xB;

	*code_section_length += c - code_start;
	*code_section_length2 += c - code_start;

	if (error_occurred)
		return 0;
	return c - wasm_binary;
}

__attribute__((export_name("get_wasm_binary")))
unsigned char *get_wasm_binary() {
	return wasm_binary;
}
