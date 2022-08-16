#include <signal.h>
#include <utility>
#include <iostream>

#include "server.h"
#include "log.h"

TLogLevel Log::reportingLevel = DEBUG;

static const char* _tag = "[HTTP SRV] ";

namespace http {
namespace server {

Server::Server(const std::string& address, const std::string& port,
               const std::string& docRoot, std::atomic<bool> &running)
    : _io_context(1),
      _acceptor(_io_context),
      _connection_manager(),
      _request_handler(docRoot, running)
{
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::resolver resolver(_io_context);
    boost::asio::ip::tcp::endpoint endpoint =
            *resolver.resolve(address, port).begin();
    _acceptor.open(endpoint.protocol());
    _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    _acceptor.bind(endpoint);
    _acceptor.listen();

    doAccept();
}

void Server::run()
{
    // The io_context::run() call will block until all asynchronous operations
    // have finished. While the Server is running, there is always at least one
    // asynchronous operation outstanding: the asynchronous accept call waiting
    // for new incoming connections.
    _io_context.run();
}

void Server::doAccept()
{
    _acceptor.async_accept(
                [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
    {
        // Check whether the Server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!_acceptor.is_open())
        {
            return;
        }

        if (!ec)
        {
            LOG_TRACE << _tag << "Receiving request from " << socket.remote_endpoint().address().to_string();
            _request_handler.setLocalEndPoint(socket.local_endpoint().address());
            _connection_manager.start(std::make_shared<Connection>(
                                          std::move(socket), _connection_manager, _request_handler));
        }

        doAccept();
    });
}

/**
 * @brief - Stop the server
 */
void Server::stop()
{
    _acceptor.close();
    _connection_manager.stop_all();
}

} // namespace Server
} // namespace http
