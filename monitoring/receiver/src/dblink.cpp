#include <boost/make_unique.hpp>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <vector>

#include "config.h"
#include "log.h"
#include "dblink.h"

static const char *_tag = "[DB] ";

namespace receiver {

DbLink::DbLink(const char* connectionString)
    : _connectionString(connectionString)
{
    auto tempString = _connectionString;
    if(tempString.length()) {
        std::vector<std::string> pairs;
        pairs = _explode(tempString, ' ');
        for(auto iit = pairs.begin(); iit != pairs.end(); ++iit) {
            std::vector<std::string> pair = _explode(*iit, '=');
            if(pair.size() == 2) {
                if(pair[0] == "dbname") {
                    _dbAlias += pair[1];
                }
                if(pair[0] == "hostaddr") {
                    _dbAlias += "@" + pair[1];
                }
            }
        }
    }
}

std::string DbLink::dbAlias() {
    return _dbAlias;
}

/**
 * @brief DbLink::connect - Connecting to database server
 * @param connectionString - connection parameters
 * @return
 */
bool DbLink::connect() {
    if(_connectionString.length()) {
        try {
            LOG_TRACE << _tag << "# Connect with: " << _connectionString;
            _connection.reset(new pqxx::connection(_connectionString));
            return _connection.get()->is_open();
        }
        catch(const std::exception &e) {
            LOG_ERROR << _tag << e.what();
        }
    }
    return false;
}

//dbname=monitdb1 user=devel password=1234 hostaddr=127.0.0.1 port=5432

std::vector<std::string> DbLink::_explode(const std::string& str, const char& ch) {
    std::string next;
    std::vector<std::string> result;

    // For each character in the string
    for (std::string::const_iterator it = str.begin(); it != str.end(); it++) {
        // If we've hit the terminal character
        if (*it == ch) {
            // If we have some characters accumulated
            if (!next.empty()) {
                // Add them to the result vector
                result.push_back(next);
                next.clear();
            }
        } else {
            // Accumulate the next character into the sequence
            next += *it;
        }
    }
    if (!next.empty())
         result.push_back(next);
    return result;
}


DbLink::DbLink(const DbLink& dblink) {
    this->_connectionString = dblink._connectionString;
    this->_connection = dblink._connection;
    this->_dbAlias = dblink._dbAlias;
}

bool DbLinkContainer::initialize(std::string configPath) {
    bool result = false;
    if(std::filesystem::is_regular_file(configPath)) {
        std::ifstream file(configPath,  std::ios::in | std::ios::binary);
        if (file.is_open()) {
            // Read contents
            const std::size_t& size = std::filesystem::file_size(configPath);
            std::string buffer(size, '\0');
            file.read(buffer.data(), size);

            std::istringstream f(buffer);
            std::string s;
            while (getline(f, s, '\n')) {
                LOG_TRACE << _tag << "Accept db conf: " << s;
                _container.insert({_container.size(), DbLink(s.c_str())});
            }
            LOG_INFO << _tag << "Accepted " << _container.size() << " connections..";
            // Close the file
            file.close();
            result = true;
        } else {
            LOG_ERROR << _tag
                      << "Error reading db conf file " << configPath;
        }
    } else {
        LOG_ERROR << _tag
                  << "Not found db conf file " << configPath;
    }
    return result;
}

const std::map<uint16_t, DbLink> &DbLinkContainer::container() {
    return _container;
}

}
