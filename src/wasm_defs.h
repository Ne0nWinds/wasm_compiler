#pragma once
enum {
	// https://webassembly.github.io/spec/core/binary/modules.html#sections
	WASM_SECTION_CUSTOM = 	0x0,
	WASM_SECTION_TYPE = 	0x1,
	WASM_SECTION_IMPORT = 	0x2,
	WASM_SECTION_FUNC = 	0x3,
	WASM_SECTION_TABLE = 	0x4,
	WASM_SECTION_MEMORY = 	0x5,
	WASM_SECTION_GLOBAL = 	0x6,
	WASM_SECTION_EXPORT = 	0x7,
	WASM_SECTION_START = 	0x8,
	WASM_SECTION_ELEMENT = 	0x9,
	WASM_SECTION_CODE = 	0xA,
	WASM_SECTION_DATA = 	0xB,
	WASM_SECTION_DATA_COUNT = 0xC,
};

enum {
	// https://webassembly.github.io/spec/core/binary/types.html#function-types
	WASM_FUNCTION_TYPE = 0x60,

	// https://webassembly.github.io/spec/core/binary/types.html#number-types
	WASM_NUMTYPE_I32 = 0x7F,
	WASM_NUMTYPE_I64 = 0x7E,
	WASM_NUMTYPE_F32 = 0x7D,
	WASM_NUMTYPE_F64 = 0x7C,
};

enum {
	WASM_I32_CONST = 0x41,
	WASM_I64_CONST = 0x42,
	WASM_F32_CONST = 0x43,
	WASM_F64_CONST = 0x44,

	WASM_I32_ADD = 0x6A,
	WASM_I32_SUB = 0x6B,
};
