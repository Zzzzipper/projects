#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <iosfwd>
#include <string>

#include "address.h"
#include "error.h"

namespace ftp::client {

Error CheckReply(
    boost::asio::ip::tcp::socket &socket,
    boost::asio::streambuf &buffer,
    int expected_code);

Error CheckTransferStatus(
    boost::asio::ip::tcp::socket &socket,
    boost::asio::streambuf &buffer);

Error GetPasvAddress(
    boost::asio::ip::tcp::socket &socket,
    boost::asio::streambuf &buffer,
    Address &address);

bool ReadLine(std::istream &stream, std::string &line);

} // namespace ftp::client
