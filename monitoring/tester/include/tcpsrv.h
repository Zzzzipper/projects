#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "config.h"
#include "dispatcher.h"

using boost::asio::ip::tcp;

namespace  tester {

class SessionManager;

/**
 * @brief The Session entity
 */
class Session
        : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket,
            SessionManager& manager, Dispatcher& dispatcher);
    virtual ~Session();

    /// Start the first asynchronous operation for the Connection.
    void start();

    /// Stop all asynchronous operations associated with the Connection.
    void stop();

private:
    /// Perfom send status
    void _sendStatus(uint32_t status);

    /// Perform an asynchronous read operation.
    void _doRead();

    /// Perform an asynchronous write operation.
    void _doWrite(std::vector<uint8_t> buffer);

    /// Socket for the Connection.
    tcp::socket _socket;

    /// The manager for this Connection.
    SessionManager& _session_manager;

    /// Buffer for incoming data.
    std::array<char, MAX_MESSAGE_LENGTH> _buffer;

    Dispatcher& _dispatcher;

    std::atomic<bool> _isAuthorized;
    std::atomic<bool> _isStopped;
    std::atomic<uint32_t> _needToRead;
    std::atomic<uint32_t> _received;

    std::vector<uint8_t> _packet;

    std::thread _responseTimer;
    std::atomic<int> _responseTick;

};

typedef std::shared_ptr<Session> session_ptr;

/**
 * @brief The SessionManager class -
 * Manages open connections so that they may be cleanly stopped when the server
 * needs to shut down.
 */
class SessionManager
{
public:
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    /// Construct a connection manager.
    SessionManager();

    /// Add the specified connection to the manager and start it.
    void start(session_ptr);

    /// Stop the specified connection.
    void stop(session_ptr);

    /// Stop all connections.
    void stop_all();

private:
    /// The managed connections.
    std::set<session_ptr> _sessions;
};

/**
 * @brief The Async TCP server entity
 */
class Server
{
public:
    explicit Server(boost::asio::io_context& io_context, unsigned short port,
                    Dispatcher& dispatcher);

private:
    /// Perform an asynchronous accept operation.
    void _doAccept(Dispatcher &dispatcher);

    /// Acceptor used to listen for incoming connections.
    tcp::acceptor _acceptor;

    /// The connection manager which owns all live connections.
    SessionManager _session_manager;
};

} // namespace  tester
