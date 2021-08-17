#include "tcpsrv.h"
#include "processor.h"
#include "json.h"
#include "m_types.h"

namespace multibase {
    /**
     * @brief session::session
     * @param socket
     */
    Session::Session(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

    /**
     * @brief session::start
     */
    void Session::start(Processor& p_) {
        doRead(p_);
    }

    /**
     * @brief session::doRead
     */
    void Session::doRead(Processor& p_) {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, MAX_MESSAGE_LENGTH),
                                [&, self](boost::system::error_code ec, std::size_t length)
        {
            try {
                if (!ec) {
                    LOG_TRACE << data_;

                    using boost::spirit::ascii::space;
                    typedef std::string::const_iterator iterator_type;
                    typedef request_parser<iterator_type> request_parser;
                    request_parser g; // Our grammar

                    std::string buffer(data_);
                    size_t startpos = buffer.find_first_not_of("}");
                    if( std::string::npos != startpos ) {
                        buffer = buffer.substr( startpos );
                    }

                    request r;
                    std::string::const_iterator iter = buffer.begin();
                    std::string::const_iterator end = buffer.end();
                    bool result = phrase_parse(iter, end, g, space, r);
                    if(result) {
                        response resp = p_.exec(r);
                        std::ostringstream streamBuf;
                        json::serializer<response>::serialize(streamBuf, resp);
                        std::string copy = streamBuf.str();
                        length = (copy.length() > MAX_MESSAGE_LENGTH)?
                                    MAX_MESSAGE_LENGTH: copy.length();
                        strncpy(data_, copy.c_str(), length);
                        data_[length - 1] = '\0';
                    }
                    doWrite(length, p_);
                }
            } catch (std::exception e) {
                LOG_ERROR << "Exception (Session::doRead): " << e.what();
            }
        });
    }

    /**
     * @brief session::doRead
     * @param length
     */
    void Session::doWrite(std::size_t length, Processor& p_) {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
                                 [&, self](boost::system::error_code ec, std::size_t /*length*/)
        {
            if (!ec) {
                doRead(p_);
            }
        });
    }

    /**
     * @brief server::server
     * @param io_context
     * @param port
     */
    Server::Server(boost::asio::io_context& io_context, unsigned short port, Processor& p)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        doAccept(p);
    }


    /**
     * @brief server::doAccept
     */
    void Server::doAccept(Processor& p) {
        acceptor_.async_accept(
                    [&](boost::system::error_code ec, tcp::socket socket)
        {
            if (!ec) {
                std::make_shared<Session>(std::move(socket))->start(p);
            }

            doAccept(p);
        });
    }

}

