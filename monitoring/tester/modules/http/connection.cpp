#include <utility>
#include <vector>
#include <iostream>

#include "connection.h"
#include "connection_manager.h"
#include "request_handler.h"

namespace http {
namespace server {

Connection::Connection(boost::asio::ip::tcp::socket socket,
                       ConnectionManager& manager, RequestHandler& handler)
    : _socket(std::move(socket)),
      _connection_manager(manager),
      _request_handler(handler)
{
}

void Connection::start()
{
    _buffer.fill('\x0');
    doRead();
}

void Connection::stop()
{
    _socket.close();
}

void Connection::doRead()
{
    auto self(shared_from_this());
    _socket.async_read_some(boost::asio::buffer(_buffer),
                            [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
    {
        if (!ec) {
            RequestParser::result_type result;
            std::tie(result, std::ignore) = _request_parser.parse(
                        _request, _buffer.data(), _buffer.data() + bytes_transferred);

            if (result == RequestParser::good) {
                _request.body.push_back('\x0');
                _request_handler.handle_request(_request, _reply);
                doWrite();
            } else if (result == RequestParser::bad) {
                _reply = reply::stock_reply(reply::bad_request);
                doWrite();
            } else {
                doRead();
            }
        } else if (ec != boost::asio::error::operation_aborted) {
            _connection_manager.stop(shared_from_this());
        }
    });
}

void Connection::doWrite()
{
    auto self(shared_from_this());
    boost::asio::async_write(_socket, _reply.to_buffers(),
                             [this, self](boost::system::error_code ec, std::size_t)
    {
        if (!ec) {
            // Initiate graceful Connection closure.
            boost::system::error_code ignored_ec;
            _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                             ignored_ec);
        }

        if (ec != boost::asio::error::operation_aborted) {
            _connection_manager.stop(shared_from_this());
        }
    });
}

} // namespace server
} // namespace http
