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

	*c++ = WASM_I32_CONST;
	*c++ = ((token *)token_list.start)[0].value;

	for (int i = 1; i < token_list.length; i += 2) {
		token t = ((token *)token_list.start)[i + 1];
		*c++ = WASM_I32_CONST;
		*c++ = t.value;

		token t2 = ((token *)token_list.start)[i];
		*c++ = (t2.type == TOKEN_PLUS) ? WASM_I32_ADD : WASM_I32_SUB;
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
