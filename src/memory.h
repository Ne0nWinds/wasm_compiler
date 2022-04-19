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


#define list_get(list, item_type, i) ((item_type *)list.start)[i]

#define list_push(list, value) {\
	typeof(value) v = value;\
	((typeof(value) *)list.start)[list.length] = v;\
	list.length += 1;\
}
