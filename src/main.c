#include "standard.h"
#include "memory.h"

static unsigned char *wasm_binary;

__attribute__((export_name("compile")))
int compile(char *text, u32 length) {
	wasm_binary = bump_alloc(1024 * 32);

	// WASM Magic Number
	wasm_binary[0] = 0;
	wasm_binary[1] = 'a';
	wasm_binary[2] = 's';
	wasm_binary[3] = 'm';

	// WASM version
	wasm_binary[4] = 1;
	wasm_binary[5] = 0;
	wasm_binary[6] = 0;
	wasm_binary[7] = 0;

	return 8;
}

__attribute__((export_name("get_wasm_binary")))
unsigned char *get_wasm_binary() {
	return wasm_binary;
}
