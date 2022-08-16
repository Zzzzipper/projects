#include "config.h"

Config::Config() {}

json::Value& Config::operator[](const std::string& name) {
    return _data[name];
}
