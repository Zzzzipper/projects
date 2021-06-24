#ifndef COMMON_PLATFORM_AVR_PLATFORM_H
#define COMMON_PLATFORM_AVR_PLATFORM_H

#include "_cplusplus.h"
#include <util/atomic.h> // For macros ATOMIC_BLOCK
#include <avr/io.h>
#include <avr/pgmspace.h>

// Пример: PROBE(K,7); Инвертирует сигнал на выходе PORTK.7
#define PROBE(x, y) {DDR##x |= MASK(y); PORT##x ^= MASK(y);}

#endif
