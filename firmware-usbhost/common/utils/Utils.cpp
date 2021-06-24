#include "include/Utils.h"

#include <string.h>

void reboot() {
	while(1);
}

const char *basename(const char *path) {
	const char *s = strrchr(path, '/');
	if(s != NULL) { return s + 1; }
	s = strrchr(path, '\\');
	if(s != NULL) { return s + 1; }
	return path;
}
