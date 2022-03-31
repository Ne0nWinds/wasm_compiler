#pragma once
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef enum bool {
	false,
	true
} bool;

#define EncodeLEB128(writeable, x, n_byte_length) {\
	typeof(x) value = x;\
	unsigned char *src = writeable;\
	unsigned char byte;\
	do {\
		byte = (value & 0x7F) | 0x80;\
		value >>= 7;\
		n_byte_length += 1;\
		*(src)++ = byte;\
	} while ((value && !(byte & 0x40)) || (value != -1 && (byte & 0x40)));\
	byte &= 0x7F;\
	*(src - 1) = byte;\
}
