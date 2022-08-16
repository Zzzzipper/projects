#pragma once

#include <string>

namespace ftp::client {

struct Address {
  std::string host;
  unsigned int port;
};

} // namespace ftp::client
