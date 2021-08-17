#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <chrono>

#include <boost/asio.hpp>

#include "config.h"
#include "tcpsrv.h"
#include "processor.h"
#include "record.h"
#include "m_types.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

/**
 * @brief The Handler class entity. Control collate statistic.
 */
class Handler
{
public:
    Handler(boost::asio::io_service& io, multibase::Processor& p)
        : strand_(io),
          timer_(io, boost::posix_time::seconds(ECHO_INTERVAL)),
          p_(p)
    {
        timer_.async_wait(strand_.wrap(boost::bind(&Handler::message, this)));
    }

    void message() {
        try {
            LOG_INFO << p_.echoStatus().str();
            timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(ECHO_INTERVAL));
            timer_.async_wait(strand_.wrap(boost::bind(&Handler::message, this)));
        } catch(std::exception& e) {
            LOG_ERROR << "Exception (Handler::message): " << e.what();
        }
    }

private:
    boost::asio::io_service::strand strand_;
    boost::asio::deadline_timer timer_;
    multibase::Processor& p_;
};

//
// Set log level. Enable output log datetime, level simbol and
// file place swithed in config.h
//
TLogLevel Log::reportingLevel = DEBUG;

int main(int argc, char* argv[])
{
    try {

        if (argc != 2) {
            LOG_ERROR << "Usage: multibase <port>\n";
            return 1;
        }

        // Create (or open) the memory mapped file where the record container
        // is stored, along with a mutex for synchronized access.
        bip::managed_mapped_file seg(
                    bip::open_or_create, "./record_container.db",
                    65536);

        multibase::Processor p(seg);
        boost::asio::io_service io_service;

        Handler h(io_service, p);
        boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

        multibase::Server s(io_service, std::atoi(argv[1]), p);

        io_service.run();

        t.join();

    } catch (std::exception& e) {
        LOG_ERROR << "Exception (main): " << e.what();
    }

    return 0;
}
