#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <string>

#include "error.h"

namespace ftp::client {

class Session {
 public:
  Session(boost::asio::ip::tcp::resolver &resolver, boost::asio::ip::tcp::socket &socket);

  // Connects to ftp server with specified credentials, on fail throws exception.
  Error Connect(const std::string &user, const std::string &password);

  // List directory and return number of files.
  Error ListDir(const std::string &path, size_t& files_count);

  // Retrieve file and return number of transfered files.
  Error Retrieve(const std::string &path, size_t& bytes_transferred);

 private:
  Error OpenDataSocket(boost::asio::ip::tcp::socket &socket);

  template <class R, class F>
  Error Transfer(const std::string& path, F consumer);

 private:
  boost::asio::ip::tcp::resolver &resolver_;
  boost::asio::ip::tcp::socket &socket_;
  boost::asio::streambuf buffer_;
};

} // namespace ftp::client
