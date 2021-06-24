#ifndef COMMON_PLATFORM_PLATFORM_H
#define COMMON_PLATFORM_PLATFORM_H

#if defined(AVR)
#include "platform/avr/include/platform.h"
#elif defined(ARM)
#include "platform/arm/include/platform.h"
#elif defined(_MSC_VER)
#include "platform/msvc/include/platform.h"
#else
#include "platform/x64/include/platform.h"
#endif

#endif
