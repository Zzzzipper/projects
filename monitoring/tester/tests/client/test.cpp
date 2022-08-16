#include <iostream>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "config.h"
#include "m_types.h"
#include "json.h"

using boost::asio::ip::tcp;

/**
 * @brief The Handler class
 */
class Handler
{
public:

    Handler(boost::asio::io_service& io, std::string host, std::string port, int delay)
        : delay_(delay),
          strand_(io),
          timer_(io, boost::posix_time::millisec(delay_)),
          host_(host),
          port_(port),
          counter_(0L)

    {
        timer_.async_wait(strand_.wrap(boost::bind(&Handler::message, this)));
    }


    /**
     * @brief iterate
     */
    void iterate() {
        const char *commands[] = {"insert", "get", "update", "get", "delete", "get"};
        static int commandNumber;
        r_.command = commands[commandNumber];
        switch(commandNumber) {
        case 0:
        case 1:
            r_.key = std::to_string(1000 + counter_);
            r_.value = "test-" + std::to_string(counter_);
            break;
        default:
            r_.key = std::to_string(1000 + counter_);
            r_.value = "testUpdated-" + std::to_string(counter_);
            break;
        }
        if(commandNumber == 5){
            commandNumber = 0;
            counter_++;
        } else {
            commandNumber++;
        }
    }

    /**
     * @brief message
     */
    void message() {
        try {
            LOG_DEBUG << ".. run test, delay " << delay_ << " msec.";

            tcp::socket s(strand_);
            tcp::resolver resolver(strand_);
            tcp::resolver::query query(host_, port_);
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

            boost::system::error_code ec;
            boost::asio::connect(s, endpoint_iterator);

            iterate();

            // Serialize command to string buffer aka simple packet
            std::ostringstream put, get;
            // json::serializer< tester::request >::serialize(put, r_);
            std::string buffer = put.str();

            LOG_DEBUG << "Request: " << buffer;
            boost::asio::write(s, boost::asio::buffer(buffer, buffer.length()));

            for (;;)  {
                boost::array<char, 128> buf;
                boost::system::error_code error;

                size_t len = s.read_some(boost::asio::buffer(buf), error);
                if (error) {
                    throw boost::system::system_error(error); // Some other error.
                }

                if(len > 0) {
                    get << buf.data();
                }

                if (error != boost::asio::error::would_block) {
                    break; // Connection closed cleanly by peer.
                }
            }

            std::string replayBuffer = get.str();

            using boost::spirit::ascii::space;
            typedef std::string::const_iterator iterator_type;
            typedef response_parser<iterator_type> response_parser;
            response_parser g; // Our grammar

            tester::response response;
            std::string::const_iterator iter = replayBuffer.begin();
            std::string::const_iterator end = replayBuffer.end();
            bool result = phrase_parse(iter, end, g, space, response);

            // TODO: bug. The parsing is sussess but return is a false ??
            // The response complete sent from server all time, not another format.
            //        if(result) {
            LOG_INFO << "Reply code: " << response.code
                     << ", what: " << response.what;

            timer_.expires_at(timer_.expires_at() + boost::posix_time::millisec(delay_));
            timer_.async_wait(strand_.wrap(boost::bind(&Handler::message, this)));

        } catch(std::exception& e) {
            LOG_ERROR << "Exception (Handler::message): " << e.what();
        }
    }

private:
    int delay_=1000;
    boost::asio::io_service::strand strand_;
    boost::asio::deadline_timer timer_;
    std::string host_;
    std::string port_;
    unsigned int counter_;
    tester::request r_;

};

/**
 * @brief test
 * @param host
 * @param port
 */
void test(const std::string host, const std::string port, int delay) {

    boost::asio::io_service io_service;

    Handler h(io_service, host, port, delay);
    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

    io_service.run();

    t.join();

}
