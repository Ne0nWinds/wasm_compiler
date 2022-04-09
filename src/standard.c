#include "standard.h"

bool string_compare(char *a, char *b, u32 n) {
	for (int i = 0; i < n; ++i) {
		if (a[i] == 0 || a[i] == 0) return false;
		if (a[i] != b[i]) return false;
	}
	return true;
}
