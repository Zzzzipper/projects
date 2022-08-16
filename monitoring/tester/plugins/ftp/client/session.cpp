#include "session.h"

#include "request.h"
#include "reply.h"
#include "scope.h"

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "session.h"

#include "request.h"
#include "reply.h"
#include "scope.h"
#include "log.h"

static const char* _tag = "[FTP::CLIENT] ";

namespace ftp::client {

Session::Session(boost::asio::ip::tcp::resolver &resolver, boost::asio::ip::tcp::socket &socket)
    : resolver_(resolver), socket_(socket) {
}

Error Session::Connect(const std::string &user, const std::string &password) {
  // Need to read hello from socket
  if (auto err = CheckReply(socket_, buffer_, 220)) {
    return err;
  }

  if (auto err = RequestUser(socket_, buffer_).Invoke(user)) {
    return err;
  }

  if (auto err = RequestPass(socket_, buffer_).Invoke(password)) {
    return err;
  }

  return Error{};
}

Error Session::ListDir(const std::string &path, size_t& files_count) {
  return Transfer<RequestList>(path, [&](boost::asio::streambuf& buffer, size_t) {
    std::istream stream(&buffer);
    std::string line;
    while (std::getline(stream, line)) {
      files_count += (line.back() == '\r');
    }
  });
}

Error Session::Retrieve(const std::string &path, size_t& bytes_transferred) {
  return Transfer<RequestRetr>(path, [&](boost::asio::streambuf&, size_t n) {
    bytes_transferred += n;
  });
}

Error Session::OpenDataSocket(boost::asio::ip::tcp::socket &socket) {
  Address address;
  if (auto err = RequestPasv(socket_, buffer_).Invoke(address)) {
    return err;
  }

  boost::system::error_code ec;
  auto endpoints = resolver_.resolve(
      address.host,
      boost::lexical_cast<std::string>(address.port),
      resolver_.numeric_host | resolver_.numeric_service,
      ec);

  if (!ec) {
    boost::asio::connect(socket, endpoints, ec);
  }
  return {ec};
}

template <class R, class F>
Error Session::Transfer(const std::string& path, F consumer) {
  boost::asio::ip::tcp::socket data_socket(socket_.get_executor());
  if (auto err = this->OpenDataSocket(data_socket)) {
    return err;
  }
  DEFER({
    data_socket.close();
  });

  if (auto err = R{socket_, buffer_}.Invoke(path)) {
    return err;
  }

  boost::system::error_code ec;
  boost::asio::streambuf response;
  while (!ec) {
    if (size_t n = boost::asio::read(data_socket, response, ec)) {
      consumer(response, n);
    }
  }

  if (ec != boost::asio::error::eof) {
    return {ec};
  }
  // Read response from control socket
  return CheckReply(socket_, buffer_, 226);
}

} // namespace ftp::client
