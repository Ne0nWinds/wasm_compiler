#include <wasm_simd128.h>
#include "standard.h"

// all string lengths when using this function must be a multiple of 16
bool string_compare(char *a, char *b, u32 n) {
	// for (int i = 0; i < n & 15; i += 16);
	__i8x16 a_vector = wasm_v128_load(a);
	__i8x16 b_vector  = wasm_v128_load(b);
	__i8x16 c = wasm_i8x16_ne(a_vector, b_vector);
	u32 bitmask = wasm_i8x16_bitmask(c);
	u32 length = __builtin_ctz(bitmask);
	return length >= n;
}
