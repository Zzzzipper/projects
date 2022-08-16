#include <boost/asio.hpp>

#include "request.h"
#include "reply.h"

namespace ftp::client {

Error Request::Send() {
  stream_ << "\r\n";
  boost::system::error_code ec;
  boost::asio::write(socket_, buffer_, ec);
  return Error{ec};
}

Error RequestUser::Invoke(const std::string &user) {
  stream_ << "USER " << user;
  if (auto err = Send()) {
    return err;
  }
  return CheckReply(socket_, buffer_, 331);
}

Error RequestPass::Invoke(const std::string &password) {
  stream_ << "PASS " << password;
  if (auto err = Send()) {
    return err;
  }
  return CheckReply(socket_, buffer_, 230);
}

Error RequestPasv::Invoke(Address &address) {
  stream_ << "PASV";
  if (auto err = Send()) {
    return err;
  }
  return GetPasvAddress(socket_, buffer_, address);
}

Error RequestList::Invoke(const std::string &path) {
  stream_ << "LIST " << path;
  if (auto err = Send()) {
    return err;
  }
  return CheckTransferStatus(socket_, buffer_);
}

Error RequestRetr::Invoke(const std::string &path) {
  stream_ << "RETR " << path;
  if (auto err = Send()) {
    return err;
  }
  return CheckTransferStatus(socket_, buffer_);
}

} // namespace ftp::client
