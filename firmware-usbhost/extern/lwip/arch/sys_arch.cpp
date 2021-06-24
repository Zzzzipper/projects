
extern "C" {
#include "lwip/include/arch/cc.h"
#include "lwip/include/lwip/sys.h"
}

#include "common/logger/include/Logger.h"
#include "common/timer/stm32/include/SystemTimer.h"

#include <stdio.h>
#include <stdarg.h>

static char buf[256];

void logPlace(const char *file, int line) {
	*(Logger::get()) << basename(file) << ":" << line << " ";
}

/**
 * Заглушка для вывода логов библиотеки LWIP
 */
void logPrintf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	*(Logger::get()) << buf << "\r";
}

/**
 * Метод используется в движке таймеров библиотеки LWIP
 */
u32_t sys_now() {
	return SystemTimer::get()->getMs();
}

