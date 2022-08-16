#pragma once

#include <boost/program_options.hpp>

#include "json.h"
#include "log.h"

#define MAX_KEY_LENGTH          1024ul
#define MAX_VALUE_LENGTH        1048576ul
#define MAX_MESSAGE_LENGTH      8192
#define MEMORY_FILE_BUF_SIZE    65536

#define TICK_INTERVAL_SEC       1   // sec.
#define TICK_INTERVAL_MSEC      300 // millisec.
#define BREAK_CONN_INTERVAL     15  // sec.
#define MAX_LOG_FILE_SIZE       2 * 1024 * 1024
#define DEF_REQUEST_PERIOD      5
#define UPLOAD_PERIOD           10


namespace po = boost::program_options;

class Config {
private:
    Config();

public:
    Config(const Config&) = delete;
    Config& operator=(const Config &) = delete;
    Config(Config &&) = delete;
    Config & operator=(Config &&) = delete;

    static auto& instance(){
        static Config tree;
        return tree;
    }


    // Debug print
    template<class Elem, class Traits>
    void print(std::basic_ostream<Elem, Traits>& aStream) {

    }

    json::Value& operator[](const std::string& name);

private:
    json::Object _data;

};


