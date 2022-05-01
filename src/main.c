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

typedef struct variable variable;
struct variable {
	char *name;
	u32 length;
	u32 memory_location;
};
static List variables = {0};

variable *find_variable(char *name, u32 length) {
	for (u32 i = 0; i < variables.length; ++i) {
		variable *v = (variable *)(variables.start) + i;

		if (v->length != length) continue;
		if (string_compare(v->name, name, length)) return v;
	}
	return 0;
}

u32 current_var_location;
variable *create_variable(char *name, u32 length) {
	current_var_location += 4;
	variable new_var = {
		.name = name,
		.length = length,
		.memory_location = current_var_location
	};
	list_push(variables, new_var);
	return ((variable *)variables.start) + (variables.length - 1);
}

u8 *expr(u8 *c) {

	typedef struct operator operator;
	struct operator {
		u8 paren_level;
		u8 precedence;
		u8 token_type;
	};

	static operator operators_data[128] = {0};

	List operators = {
		.length = 0,
		.start = operators_data
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
			variable *v = find_variable(t.identifier.name, t.identifier.length);
			if (!v)
				v = create_variable(t.identifier.name, t.identifier.length);

			*c++ = WASM_I32_CONST;
			*c++ = v->memory_location;
			if ((current_token + 1)->type == TOKEN_ASSIGN) {
				*c++ = WASM_I32_CONST;
				*c++ = v->memory_location;
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
			if (current_paren_level == 0) break;

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

void expect_token(token_type type) {
	if (current_token->type == type) {
		current_token += 1;
	} else {
		error_occurred = true;
	}
}

u8 *expr_stmt(u8 *c) {
	u8 *expr_code = expr(c);
	expect_token(TOKEN_SEMICOLON);
	return expr_code;
}

u8 *compound_stmt(u8 *c) {
	depth = 0;

	enum {
		BLOCK_NORMAL,
		BLOCK_IF,
		BLOCK_ELSE,
		BLOCK_FOR,
	};

	typedef struct code code;
	struct code {
		u8 *code;
		u32 length;
	};

	typedef struct code_block code_block;
	struct code_block {
		u32 type;
		union {
			struct {
				code condition;
				code iteration;
			} _for;
		};
	};

	static code_block code_blocks_stack_data[128] = {0};

	List code_blocks_stack = {
		.length = 0,
		.start = code_blocks_stack_data
	};


	code_block b = {BLOCK_NORMAL};

	expect_token(TOKEN_OPEN_BRACKET);
	list_push(code_blocks_stack, (code_block){BLOCK_NORMAL});

	bump_move(sizeof(code_block));

	while (current_token - (token *)token_list.start < token_list.length && !error_occurred) {
		if (current_token->type == TOKEN_OPEN_BRACKET) {
			current_token += 1;
			list_push(code_blocks_stack, (code_block){BLOCK_NORMAL});
			bump_move(sizeof(code_block));
			continue;
		}

		if (current_token->type == TOKEN_CLOSED_BRACKET || current_token->type == TOKEN_SEMICOLON) {

			code_block block;
			block = list_get(code_blocks_stack, code_block, code_blocks_stack.length - 1);

			if (current_token->type == TOKEN_SEMICOLON) {
				if (block.type == BLOCK_NORMAL) {
					current_token += 1;
					continue;
				}
			}

			if (current_token->type == TOKEN_CLOSED_BRACKET) {
				if (block.type == BLOCK_NORMAL) {
					code_blocks_stack.length -= 1;
				} else {
					error_occurred = true;
					break;
				}
			}

			current_token += 1;

			if (code_blocks_stack.length == 0) continue;

			block = list_get(code_blocks_stack, code_block, code_blocks_stack.length - 1);

			while (block.type != BLOCK_NORMAL && code_blocks_stack.length) {
				if (block.type == BLOCK_ELSE) {
					*c++ = WASM_END;
					code_blocks_stack.length -= 1;
				} else if (block.type == BLOCK_FOR) {
					__builtin_memcpy(c, block._for.iteration.code, block._for.iteration.length);
					c += block._for.iteration.length;

					__builtin_memcpy(c, block._for.condition.code, block._for.condition.length);
					c += block._for.condition.length;
					*c++ = WASM_BR_IF;
					*c++ = 0;

					*c++ = WASM_END;
					*c++ = WASM_END;
					code_blocks_stack.length -= 1;
				} else if (block.type == BLOCK_IF && current_token->type != TOKEN_ELSE) {
					*c++ = WASM_END;
					code_blocks_stack.length -= 1;
				} else {
					break;
				}
				block = list_get(code_blocks_stack, code_block, code_blocks_stack.length - 1);
			}
			continue;
		}

		if (current_token->type == TOKEN_IF) {
			current_token += 1;
			expect_token(TOKEN_OPEN_PARENTHESIS);
			c = expr(c);
			depth -= 1;
			expect_token(TOKEN_CLOSED_PARENTHESIS);
			*c++ = WASM_IF;
			*c++ = 0x40; // this may have to be changed

			code_block b = {0};
			b.type = BLOCK_IF;
			list_push(code_blocks_stack, b);
			bump_move(sizeof(code_block));
			continue;
		}

		if (current_token->type == TOKEN_ELSE) {
			current_token += 1;
			*c++ = WASM_ELSE;
			code_blocks_stack.length -= 1;
			code_block prev_block = list_get(code_blocks_stack, code_block, code_blocks_stack.length - 1);

			// if (prev_block.type != BLOCK_IF || prev_block.type != STMT_IF) error_occurred = true;

			code_block b = {0};
			b.type = BLOCK_ELSE;
			list_push(code_blocks_stack, b);
			bump_move(sizeof(code_block));
			continue;
		}

		if (current_token->type == TOKEN_FOR || current_token->type == TOKEN_WHILE) {
			token_type loop_type = current_token->type;
			current_token += 1;
			expect_token(TOKEN_OPEN_PARENTHESIS);

			u32 condition_length = 0;
			u8 *condition = 0;

			u32 iteration_length = 0;
			u8 *iteration = 0;

			if (loop_type == TOKEN_FOR) {

				// init
				c = expr_stmt(c);
				if (depth > 0) {
					*c++ = WASM_DROP;
					depth -= 1;
				}

				// condition
				condition = bump_get();
				u8 *condition_end = expr_stmt(condition);
				depth -= 1;
				condition_length = condition_end - condition;
				bump_move(condition_length);

				// iteration
				iteration = bump_get();
				u8 *iteration_end = expr(iteration);
				*iteration_end++ = WASM_DROP;
				depth -= 1;
				iteration_length = iteration_end - iteration;
				bump_move(iteration_length);
			} else if (loop_type == TOKEN_WHILE) {
				condition = bump_get();
				u8 *condition_end = expr(condition);
				depth -= 1;
				condition_length = condition_end - condition;
				bump_move(condition_length);
			}

			expect_token(TOKEN_CLOSED_PARENTHESIS);

			code_block for_block = {0};
			for_block.type = BLOCK_FOR;
			for_block._for.condition.code = condition;
			for_block._for.condition.length = condition_length;
			for_block._for.iteration.code = iteration;
			for_block._for.iteration.length = iteration_length;

			list_push(code_blocks_stack, for_block);

			*c++ = WASM_BLOCK;
			*c++ = 0x40;
			__builtin_memcpy(c, condition, condition_length);
			c += condition_length;
			*c++ = WASM_I32_CONST;
			*c++ = 1;
			*c++ = WASM_I32_NE;
			*c++ = WASM_BR_IF;
			*c++ = 0;
			*c++ = WASM_LOOP;
			*c++ = 0x40;

			continue;
		}

		bool should_return = false;
		if (current_token->type == TOKEN_RETURN) {
			current_token += 1;
			should_return = true;
		}

		c = expr(c);

		if (should_return) {
			*c++ = WASM_RETURN;
			depth -= 1;
		}

		if (depth > 0) {
			*c++ = WASM_DROP;
			depth -= 1;
		}

	}

	if (depth == 0) {
		*c++ = WASM_I32_CONST;
		*c++ = 0;
	}

	if (code_blocks_stack.length != 0)
		error_occurred = true;

	bump_free(code_blocks_stack.start);

	if (depth != 0)
		c -= 1;
	return c;
}

__attribute__((export_name("compile")))
int compile(char *text, u32 length) {

	current_var_location = 4;
	variables.length = 0;
	variables.start = bump_alloc(128 * sizeof(variable));

	error_occurred = false;
	wasm_binary = bump_alloc(1024 * 32);

	u8 *c = wasm_binary;
	c = wasm_header(c);
	c = create_main_function(c);

	bool unexpected_token = false;
	token_list = tokenize_text(text, length, &unexpected_token);

	current_token = token_list.start;

	// c[0] = WASM_SECTION_CODE;
	// c[1] = 0x3;
	// c[2] = 0x1; // vec(code)
	// c[3] = 0x1;
	// c[4] = 0x0;

	u8 *code_section_header = c;

	u8 *code_start = c;

	c = compound_stmt(c);

	*c++ = WASM_END;

	u8 code_length1[4] = {0};
	u8 code_length2[4] = {0};

	u32 byte_length2 = 0;
	EncodeLEB128(code_length2, c - code_start + 1, byte_length2);
	u32 byte_length1 = 0;
	EncodeLEB128(code_length1, c - code_start + 2 + byte_length2, byte_length1);

	u32 section_header_length = 3 + byte_length1 + byte_length2;
	__builtin_memcpy(code_section_header + section_header_length, code_section_header, c - code_start);

	*code_section_header++ = WASM_SECTION_CODE;
	__builtin_memcpy(code_section_header, code_length1, byte_length1);
	code_section_header += byte_length1;
	*code_section_header++ = 0x1; // vec(code)
	__builtin_memcpy(code_section_header, code_length2, byte_length2);
	code_section_header += byte_length2;
	*code_section_header++ = 0;

	if (error_occurred)
		return 0;

	c += section_header_length;
	return c - wasm_binary;
}

__attribute__((export_name("get_wasm_binary")))
unsigned char *get_wasm_binary() {
	return wasm_binary;
}
