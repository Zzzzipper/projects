//
// server.hpp
//

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include "connection.h"
#include "connection_manager.h"
#include "request_handler.h"
#include "dispatcher.h"
#include "holder.h"

namespace http {
namespace server {

/**
 * @brief The Server class - Entity of The top-level class
 *                           of the HTTP server.
 */
class Server
{
public:
  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit Server(const std::string&, const std::string&,
      const std::string&, std::atomic<bool>&);

  /// Run the server's io_context loop.
  void run();

  /// Stop the server.
  void stop();

private:
  /// Perform an asynchronous accept operation.
  void doAccept();

  /// The io_context used to perform asynchronous operations.
  boost::asio::io_context _io_context;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor _acceptor;

  /// The connection manager which owns all live connections.
  ConnectionManager _connection_manager;

  /// The handler for all incoming requests.
  RequestHandler _request_handler;

};

} // namespace server
} // namespace http

#endif // HTTP_SERVER_HPP
