#pragma once
#include "standard.h"

void *bump_alloc(u32 s);

void bump_free(void *last);

u32 bump_alloc_js(u32 s);
