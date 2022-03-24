#include "memory.h"

static char *memory[1024 * 32];
static char *current = (char *)memory;

// TODO: memory alignment
void *bump_alloc(u32 s) {
	void *ptr = current;
	__builtin_memset(ptr, 0, s);
	current += s;
	return ptr;
}

__attribute__((export_name("bump_alloc_js")))
u32 bump_alloc_js(u32 s) {
	return (u32)bump_alloc(s);
}

__attribute__((export_name("bump_reset_js")))
void bump_reset_js() {
	bump_reset();
}

void *bump_get() {
	return current;
}
void bump_move(u32 s) {
	current += s;
}

void bump_free(void *last) {
	current = last;
}
void bump_reset() {
	current = (char *)memory;
}
