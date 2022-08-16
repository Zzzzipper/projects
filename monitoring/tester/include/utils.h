#pragma once

#include <inttypes.h>
#include <sstream>

#include "json.h"

/// Convert string ipv4 address to unsigned 4-bytes integer
uint32_t ipStringToNumeric( const std::string& ipv4Str);

/// Convert numeric IP address tu string
std::string ipNumericToString(uint32_t ip);

/// Extract domain address or string
std::string extractDomainName(std::string url);

/// Netstring to Json object parser
json::Object parseNetStringToJsonObject(std::string input);

/// Get GET parameters from uri
std::string getParametersFromUri(std::string uri);
