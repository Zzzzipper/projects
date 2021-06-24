#ifndef COMMON_UTILS_UTILS_H_
#define COMMON_UTILS_UTILS_H_

extern void reboot();
#ifndef __linux__
extern const char *basename(const char *path);
#endif

#endif
