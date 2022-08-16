#pragma once

#include <boost/system/error_code.hpp>

#include <string>

namespace ftp::client {

class Error {
 public:
  Error() : code_(0) {}
  Error(std::string message) : code_(1), message_(std::move(message)) {}
  Error(int code, std::string message) : code_(code), message_(std::move(message)) {}
  Error(const boost::system::error_code &ec) : code_(ec.value()), message_(ec.message()) {}

  explicit operator bool() {
    return 0 != code_;
  }

  int code() const {
    return code_;
  }

  const std::string &message() const {
    return message_;
  }

 private:
  int code_;
  std::string message_;
};

} // namespace ftp::client
