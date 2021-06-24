#ifndef _LWIP_ARCH_CC_H
#define _LWIP_ARCH_CC_H

#include <stdarg.h>

extern void logPlace(const char *file, int line);
extern void logPrintf(const char *format, ...);

#define LWIP_PLATFORM_ASSERT(x) logPlace(__FILE__, __LINE__); logPrintf(x); while(1)
#define LWIP_PLATFORM_DIAG(x) logPlace(__FILE__, __LINE__); logPrintf x

#endif
