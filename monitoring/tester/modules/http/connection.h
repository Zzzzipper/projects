#pragma once

#include <array>
#include <memory>
#include <boost/asio.hpp>
#include "reply.h"
#include "request.h"
#include "request_handler.h"
#include "request_parser.h"
#include "dispatcher.h"

namespace http {
namespace server {

class ConnectionManager;

/// Represents a single Connection from a client.
class Connection
  : public std::enable_shared_from_this<Connection>
{
public:
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  /// Construct a Connection with the given socket.
  explicit Connection(boost::asio::ip::tcp::socket socket,
      ConnectionManager& manager, RequestHandler& handler);

  /// Start the first asynchronous operation for the Connection.
  void start();

  /// Stop all asynchronous operations associated with the Connection.
  void stop();

private:
  /// Perform an asynchronous read operation.
  void doRead();

  /// Perform an asynchronous write operation.
  void doWrite();

  /// Socket for the Connection.
  boost::asio::ip::tcp::socket _socket;

  /// The manager for this Connection.
  ConnectionManager& _connection_manager;

  /// The handler used to process the incoming request.
  RequestHandler& _request_handler;

  /// The parser for the incoming request.
  RequestParser _request_parser;

  /// Buffer for incoming data.
  std::array<char, 8192> _buffer;

  /// The incoming request.
  request _request;

  /// The reply to be sent back to the client.
  reply _reply;

};

typedef std::shared_ptr<Connection> connection_ptr;

} // namespace server
} // namespace http

