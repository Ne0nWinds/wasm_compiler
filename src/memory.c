#include "memory.h"

static char *memory[1024 * 32];
static char *current = (char *)memory;

// TODO: memory alignment
void *bump_alloc(u32 s) {
	current += s;
	return current;
}

__attribute__((export_name("bump_alloc_js")))
u32 bump_alloc_js(u32 s) {
	void *ptr = bump_alloc(s);
	return (u32)ptr;
}

void bump_free(void *last) {
	current = last;
}
