#pragma once
#include "standard.h"

void *bump_alloc(u32 s);

void bump_free(void *last);

void *bump_get();
void bump_move(u32 s);
void bump_reset();

typedef struct List List;
struct List {
	u32 length;
	void *start;
};

#define list_add(list, item) {\
	typeof(item) *start = list.start; \
	start[list.length] = item; \
	list.length += 1; \
}
