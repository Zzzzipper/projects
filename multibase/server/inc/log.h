/**
 *  Simple switched stdout/stderr logger version 0.1
 */
#pragma once

#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <string>

enum TLogLevel {
    ERROR,
    WARNING,
    INFO,
    DEBUG,
    TRACE
};

/**
 * @brief The Log entity
 */
class Log {
public:
    Log() = default;
    /**
     * @brief ~Log
     */
    virtual ~Log() {
        if (_messageLevel <= Log::ReportingLevel()) {
            os << std::endl;
            std::cerr << os.str();
        }
    }

    /**
     * @brief Get
     * @param level
     * @return
     */
    std::ostringstream& Get(TLogLevel level = INFO) {
        _messageLevel = level;
        return os;
    }

public:
    static TLogLevel& ReportingLevel() {
        return reportingLevel;
    }

    static TLogLevel reportingLevel;

protected:
    std::ostringstream os;

private:
    Log(const Log&) = delete;
    Log& operator =(const Log&) = delete;

private:
    TLogLevel _messageLevel;
};

/**
 * @brief Time
 * @return
 */
inline std::string Time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%X %d:%m:%Y");
    return ss.str();
}
/**
 * @brief Level
 * @param level
 * @return
 */
inline const char* Level(TLogLevel level) {
    switch(level) {
    case ERROR:     return "(E)";
    case WARNING:   return "(W)";
    case INFO:      return "(I)";
    case DEBUG:     return "(D)";
    case TRACE:     return "(T)";
    default:
        break;
    }
    return "(*)";
}

// Nicers
// If needed - log line code place
#ifdef LOG_FILE
    #define OUT_LINE            << ":" << basename(__FILE__) << "#" << __LINE__<< " "
#else
    #define OUT_LINE
#endif
// If needed -  log time
#ifdef LOG_TIME
    #define OUT_TIME            << "[" << Time() << "] "
#else
    #define OUT_TIME
#endif
// If needed - log level
#ifdef LOG_LEVEL
    #define OUT_LEVEL(level)    << Level(level) << " "
#else
    #define OUT_LEVEL(level)
#endif

#define LOG(level)      Log().Get(level) \
                        OUT_LEVEL(level) OUT_TIME OUT_LINE \
                        << ((level > DEBUG)? std::string(static_cast<size_t>(level - DEBUG), '\t'): " ")

#define LOG_ERROR       LOG(ERROR)
#define LOG_WARNING     LOG(WARNING)
#define LOG_INFO        LOG(INFO)
#define LOG_DEBUG       LOG(DEBUG)
#define LOG_TRACE       LOG(TRACE)



