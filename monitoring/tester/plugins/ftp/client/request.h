#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <ostream>
#include <string>

#include "address.h"
#include "error.h"

namespace ftp::client {

class Request {
 public:
  Request(boost::asio::ip::tcp::socket &socket, boost::asio::streambuf &buffer)
      : socket_(socket), buffer_(buffer), stream_(&buffer_) {
  }

  Error Send();

 protected:
  boost::asio::ip::tcp::socket &socket_;
  boost::asio::streambuf &buffer_;
  std::ostream stream_;
};

// Send user
class RequestUser : Request {
 public:
  using Request::Request;

  Error Invoke(const std::string &user);

};

// Send password
class RequestPass : Request {
 public:
  using Request::Request;

  Error Invoke(const std::string &password);

};

// Request switch to passive mode
class RequestPasv : Request {
 public:
  using Request::Request;

  Error Invoke(Address &address);
};

// List of directory
class RequestList : Request {
 public:
  using Request::Request;

  Error Invoke(const std::string &path);
};

// Retrieve file
class RequestRetr : Request {
 public:
  using Request::Request;

  Error Invoke(const std::string &path);
};

} // namespace ftp::client
