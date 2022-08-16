#include <inttypes.h>
#include <sstream>
#include <regex>
#include <iostream>

#include "utils.h"


/**
 * @brief convert - Convert string ipv4 address to unsigned 4-bytes integer
 * @param ipv4Str
 * @return
 */
uint32_t ipStringToNumeric( const std::string& ipv4Str ) {
    std::istringstream iss( ipv4Str );

    uint32_t ipv4 = 0;

    for( uint32_t i = 0; i < 4; ++i ) {
        uint32_t part;
        iss >> part;
        if ( iss.fail() || part > 255 ) {
            throw std::runtime_error( "Invalid IP address - Expected [0, 255]" );
        }

        // LSHIFT and OR all parts together with the first part as the MSB
        ipv4 |= part << ( 8 * ( 3 - i ) );

        // Check for delimiter except on last iteration
        if ( i != 3 ) {
            char delimiter;
            iss >> delimiter;
            if ( iss.fail() || delimiter != '.' ) {
                throw std::runtime_error( "Invalid IP address - Expected '.' delimiter" );
            }
        }
    }

    return ipv4;
}

/**
 * @brief ipNumericToString - Convert numeric IP address tu string
 * @param ip
 * @return
 */
std::string ipNumericToString(uint32_t ip) {
    std::stringstream ss;
    ss << (int)(ip & 0xFF)
       << "." <<  (int)((ip >> 8) & 0xFF)
       << "." <<  (int)((ip >> 16) & 0xFF)
       << "." <<  (int)((ip >> 24) & 0xFF);
    return ss.str();
}

/**
 * @brief extractDomainName - Extract domain address or string
 * @param url
 * @return
 */
std::string extractDomainName(std::string url) {

    std::regex regex("^(?:https?:\\/\\/)?(?:[^@\\/\\n]+@)?(?:www\\.)?([^:\\/?\\n]+)");

    std::smatch match;
    const std::string local = url;

    if (std::regex_search(local.begin(), local.end(), match, regex)) {
        return match[1];
    }

    return {};
}

/**
 * @brief parseNetStringToJsonObject - Netstring to Json object parser
 * @param input
 * @return
 */
json::Object parseNetStringToJsonObject(std::string input) {

    if(input.length() == 0) {
        return {};
    }

    json::Object data;

    int cursor = 0;

    auto sizeLen = std::atoi(input.substr(cursor, 1).c_str());
    cursor++;

    auto count = std::atoi(input.substr(cursor, sizeLen).c_str());
    cursor+=sizeLen;

    auto order = 0;

    std::string key, string;
    for(int i = 0; i < count; ++i) {
        auto sizeLen = std::atoi(input.substr(cursor, 1).c_str());
        cursor++;

        auto length = std::atoi(input.substr(cursor, sizeLen).c_str());
        cursor+=sizeLen;

        if(order == 0) {
            key = input.substr(cursor, length).c_str();
            order = 1;
        } else if(order == 1) {
            string = input.substr(cursor, length).c_str();
            data[key] = string;
            order = 0;
        }
        cursor += length;
    }

    return data;

}

/**
 * @brief getParametersFromUri - Get GET parameters from uri
 * @param uri
 * @return
 */
std::string getParametersFromUri(std::string uri) {

    std::regex regex("^[^?#]+\\?([^#]+)");

    std::smatch match;
    const std::string local = uri;

    if (std::regex_search(local.begin(), local.end(), match, regex)) {
        return match[1];
    }

    return {};
}




