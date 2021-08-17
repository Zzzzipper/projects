#pragma once

//
// Logger setup defs
//

#define ENABLE_LOGGGING

#ifdef ENABLE_LOGGGING
//    #define LOG_TIME    // Enable time out in log line
    #define LOG_LEVEL   // Enable show log level as single liters in log line
//    #define LOG_FILE    // Enable output code line in file where log is calling
#endif

#include "log.h"

#define MAX_KEY_LENGTH      1024ul
#define MAX_VALUE_LENGTH    1048576ul
#define MAX_MESSAGE_LENGTH  1049600UL

#define ECHO_INTERVAL       2 // sec.
