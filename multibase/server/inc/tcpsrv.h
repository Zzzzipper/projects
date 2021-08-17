#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "config.h"

using boost::asio::ip::tcp;

namespace  multibase {

    class Processor;

    /**
     * @brief The The Session entity
     */
    class Session
            : public std::enable_shared_from_this<Session>
    {
    public:
        Session(tcp::socket socket);
        void start(Processor& p);

    private:
        void doRead(Processor &p);
        void doWrite(std::size_t length, Processor &p);

        tcp::socket socket_;
        char data_[MAX_MESSAGE_LENGTH];
    };

    /**
     * @brief The The Async TCP server entity
     */
    class Server
    {
    public:
        Server(boost::asio::io_context& io_context, unsigned short port, Processor& p);

    private:
        void doAccept(Processor& p);
        tcp::acceptor acceptor_;
    };


}
