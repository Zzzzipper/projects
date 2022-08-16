#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <chrono>

#include <boost/asio.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>

#include "config.h"
#include "tcpsrv.h"
#include "record.h"
#include "m_types.h"
#include "watchdog.h"
#include "ver.h"
#include "increment.h"
#include "requestor.h"
#include "sender.h"
#include "dispatcher.h"
#include "stamp.h"
#include "timer.h"
#include "dblink.h"

#include "server.h"

namespace po = boost::program_options;

static const char* _tag = "[MAIN] ";

static Config & _config = Config::instance();

/**
 * @brief The Handler class entity. Control collate statistic.
 */
class Handler
{
public:
    Handler(boost::asio::io_service& io/*, tester::Processor& p*/)
        : strand_(io),
          timer_(io, boost::posix_time::seconds(TICK_INTERVAL_SEC))/*,
          p_(p)*/
    {
        timer_.async_wait(strand_.wrap(boost::bind(&Handler::message, this)));
    }

    void message() {
        try {
//            LOG_INFO << _tag << p_.echoStatus().str();
            timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(TICK_INTERVAL_SEC));
            timer_.async_wait(strand_.wrap(boost::bind(&Handler::message, this)));
        } catch(std::exception& e) {
            LOG_ERROR << _tag << "Exception (Handler::message): " << e.what();
        }
    }

private:
    boost::asio::io_service::strand strand_;
    boost::asio::deadline_timer timer_;
//    tester::Processor& p_;
};

#if 0
int _test_timer_count = 0;
void _test_timer_handler() {
    _test_timer_count++;
    std::cout << "_test_timer_handler\n";
}
#endif

/**
 * @brief port_in_use - check port in use
 * @param port
 * @return
 */

/// TODO: вынести в utils
static bool port_in_use(unsigned short port) {
    using namespace boost::asio;
    using ip::tcp;

    io_service svc;
    tcp::acceptor a(svc);

    boost::system::error_code ec;
    a.open(tcp::v4(), ec) || a.bind({ tcp::v4(), port }, ec);

    return ec == error::address_in_use;
}


//
// Set log level. Enable output log datetime, level simbol and
// file place swithed in config.h
//
TLogLevel Log::reportingLevel = DEBUG;

#if 0
int main(int argc, char* argv[])
{
    try {

        tester::Runnable runnable;

        assert(!runnable.is_active());
        assert(runnable.get_interval() == 0);

        runnable.set_interval(1000);
        runnable.connect(_test_timer_handler);

        runnable.start();

        assert(runnable.is_active());

        std::cout << "timer test started\n";

        boost::this_thread::sleep<boost::posix_time::milliseconds>(boost::posix_time::milliseconds(5500));

        runnable.stop();

        assert(!runnable.is_active());
        assert(_test_timer_count == 5);


    } catch (std::exception& e) {
        LOG_ERROR << "Exception (main): " << e.what();
    }

    return 0;
}

#else

///
/// Shutdown handler
///
std::function<void(int)> shutdown_handler;
void signal_handler(int signal) {
    shutdown_handler(signal);
}

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[])
{
    int error = 0;

    try {

        po::options_description desc("Allowed options:");
        desc.add_options()
                ("help", "produce help message")
                ("p", po::value<int>(), "receiver listening TCP port (default 10001)")
                ("t", po::value<int>(), "request tick delay, sec. (default 30)")
                ("r", po::value<int>(), "default testers reread tasks period, sec. (default 60")
                ("w", po::value<int>(), "http server port (default 8080)")
                ("h", po::value<std::string>(), "htdocs directory (default ./)")
                ("n", po::value<std::string>(), "receiver nickname (default 'Receiver')")
                ("l", po::value<std::string>(), "receiver log path (default .)")
                ("d", po::value<std::string>(), "database connection config file path (default ./db.conf)");

        //"dbname = testdb user = postgres password = cohondob \
        hostaddr = 127.0.0.1 port = 5432"

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            error = 200;
            goto close;
        }

        _config["logPath"] = ".";
        if (vm.count("l")) {
            _config["logPath"] = vm["l"].as<std::string>();
        }

        //
        // Enable file logger
        //
        FileLogger::instance().enableFileLog(_config["logPath"].ToString().c_str());

        LOG_INFO << _tag << "# log path was set to " << _config["logPath"].ToString();

        std::cout << "###########################################################" << std::endl;
        std::cout << "# Start "  << PROJECT_NAME
                  << ", ver.: "  << PROJECT_VER
                  << ", build: " << INCREMENTED_VALUE << std::endl;
        std::cout << "#" << std::endl;

        _config["reReadPeriod"] = 60;
        if (vm.count("r")) {
            _config["reReadPeriod"] = vm["r"].as<int>();
        }
        LOG_INFO << _tag << "# default testers reread tasks period was set to " << _config["reReadPeriod"].ToInt();


        _config["requestDelay"] = 5;
        if (vm.count("t")) {
            _config["requestDelay"] = vm["t"].as<int>();
        }
        LOG_INFO << _tag << "# request tick delay was set to " << _config["requestDelay"].ToInt();

        //------------------------------------------------------------

        _config["appName"] = "Receiver";
        if (vm.count("n")) {
            _config["appName"] = vm["n"].as<std::string>();
        }
        LOG_INFO << _tag << "# receiver nickname was set " << _config["appName"].ToString();


        _config["port"] = 10001;
        if (vm.count("p")) {
            _config["port"] = vm["p"].as<int>();
        }
        LOG_INFO  << _tag << "# receiver listening TCP port was set to " << _config["port"].ToInt();


        _config["htdocs"] = "./";

        if (vm.count("h")) {
            _config["htdocs"] = vm["h"].as<std::string>();
        }
        LOG_INFO << _tag << "# htdocs directory was set to " << _config["htdocs"].ToString();

        _config["dbconf"] = "./db.conf";
        if (vm.count("d")) {
            _config["dbconf"] = vm["d"].as<std::string>();
        }
        LOG_INFO << _tag << "# db.conf path was set to " << _config["dbconf"].ToString();
        if(!receiver::DbLinkContainer::instance().initialize(_config["dbconf"].ToString()))  {
            LOG_ERROR << _tag << "DbLink has error..";
        }


        //------------------------------------------------------------
        // Module thread array
        std::vector<boost::thread> many;
        std::atomic<bool> running(true);

        boost::asio::io_service ioService;

        //------------------------------------------------------------
        // Ticker loop
        //

        tester::Timer t;

        many.emplace_back([&running, &t] {
            LOG_INFO << _tag << "Start tick thread..";
            while(running) {
                std::this_thread::sleep_for(std::chrono::seconds(TICK_INTERVAL_SEC));
                LOG_TRACE << "[TICK] " << "..";
                t.tick();
            }
            LOG_INFO << _tag << "Tick thread stopped..";
        });

        auto needToLaunch = false;


        _config["httpPort"] = 8080;
        if (vm.count("w")) {
            try {
                _config["httpPort"] = vm["w"].as<int>();
            } catch(std::exception &e) {
                LOG_ERROR << _tag << e.what();
            }
        }

        while(port_in_use(_config["httpPort"].ToInt())) {
            _config["httpPort"] = _config["httpPort"].ToInt() + 10;
        }

        LOG_INFO << _tag << "# http server port was set to " << _config["httpPort"].ToInt();

        // Set tester nickname
        tester::Dispatcher::instance().setName(_config["appName"]);
        // Set tester version value
        auto strVersion = std::string(PROJECT_VER)
                + "." + std::to_string(INCREMENTED_VALUE);
        tester::Dispatcher::instance().setVersion(strVersion);

        std::stringstream stringStream;
        stringStream << _config["appName"].ToString()
                     << " v"             << strVersion
                     << ", Compiled: "   << COMPILE_TIME
                     << ", Git commit: " << GIT_COMMIT
                     << ", Time: "       << GIT_COMMIT_TIME;

        tester::Dispatcher::instance().setInfo(stringStream.str());

        http::server::Server server("0.0.0.0", std::to_string(_config["httpPort"].ToInt()),
                _config["htdocs"], running);

        //------------------------------------------------------------
        //  Catch SIGINT
        //

        struct sigaction sigIntHandler;
        sigemptyset(&sigIntHandler.sa_mask);
        sigIntHandler.sa_flags = 0;

        shutdown_handler = [&](int signal) {
            static bool closeStarted;
            if(!closeStarted) {
                LOG_DEBUG << _tag << "Caught signal " << signal;
                running = false;
                closeStarted = true;

                server.stop();
                ioService.stop();
            }
        };

        sigIntHandler.sa_handler = signal_handler;

        sigaction(SIGINT, &sigIntHandler, NULL);


        //------------------------------------------------------------
        // Httpsrv module loop
        //

        many.emplace_back([&] {
            while(running) {
                try {
                    LOG_INFO << _tag << "Start HTTP Server module..";
                    server.run();
                    running = false;
                }
                catch(boost::system::system_error &er) {
                    LOG_ERROR << _tag << "Asio exception error: " << er.what() << ", code " << er.code();
                    if(er.code().value() == 98 || er.code().value() == -8) {
                        LOG_ERROR << _tag << "Web module not running..";
                        break;
                    }
                }
                catch(std::exception e) {
                    LOG_ERROR << _tag << "Exception " << e.what();
                }
            }
            LOG_ERROR << _tag << "Web module stopped..";
            ioService.stop();
        });

        //------------------------------------------------------------
        // Requestor loop
        //
#if 0
        many.emplace_back([&] {

            tester::Requestor requestor;
            t.setObserver(&requestor);

            while(running) {
                if(requestor.isReady(_config["requestDelay"].ToInt())) {
                    try {
                        LOG_INFO << _tag << "Start request tasks from testers..";
                        // Initialise the requestor.
                        // requestor.requestTasks(receiverAddrPair[0].c_str(), receiverAddrPair[1].c_str());
                        if(!running) {
                            return;
                        }
                        if(tester::Dispatcher::instance().getExtConfig().CfgRereadPeriod > 0) {
                            _config["requestDelay"] = (int)tester::Dispatcher::instance().getExtConfig().CfgRereadPeriod;
                            LOG_WARNING << _tag << "Request delay was set to " << _config["requestDelay"].ToInt();
                        }
                    } catch(std::runtime_error e) {
                        LOG_ERROR << _tag << "Exception " << e.what();
                        LOG_ERROR << _tag << "Requestor get tasks failed..";
                    }
                }
                std::this_thread::sleep_for(std::chrono::seconds(TICK_INTERVAL));
            }
            LOG_INFO << _tag << "Config requestor stopped..";
        });

        //------------------------------------------------------------
        // Sender loop
        //
        many.emplace_back([&running, &t] {

            tester::Sender sender;
            t.setObserver(&sender);

            while(running) {
                if(sender.isReady(30)) {
                    try {
                        LOG_INFO << _tag << "Start upload result to receiver..";
                        // Initialise the requestor.
                        // sender.uploadResults(receiverAddrPair[0].c_str(), receiverAddrPair[1].c_str());
                        if(!running) {
                            return;
                        }
                    } catch(std::exception e) {
                        LOG_ERROR << _tag << "Exception " << e.what();
                        LOG_ERROR << _tag << "Sender send results failed..";
                    }
                }
                std::this_thread::sleep_for(std::chrono::seconds(TICK_INTERVAL));
            }
            LOG_INFO << _tag << "Result sender stopped..";
        });
#endif
        //------------------------------------------------------------
        // Listening loop
        //

        many.emplace_back([&] {

            while(running) {
                try {
                    LOG_INFO << _tag << "Start receiver listening server on " << _config["port"].ToInt() << " port..";
                    tester::Server server(ioService, _config["port"].ToInt(), tester::Dispatcher::instance());
                    ioService.reset();
                    ioService.run();
                } catch (std::exception &e) {
                    LOG_ERROR << _tag << "Exception " << e.what();
                }
            }

            LOG_INFO << _tag << "Listening server stopped..";

        });

        //------------------------------------------------------------
        // Wait until all action ends...
        //
        for (auto& t : many) {
            t.join();
        }

#if 0
        // Create (or open) the memory mapped file where the record container
        // is stored, along with a mutex for synchronized access.
        bip::managed_mapped_file seg(
                    bip::open_or_create, dbpath.c_str(),
                    MEMORY_FILE_BUF_SIZE);

        tester::Processor p(seg);
        boost::asio::io_service io_service;

        Handler h(io_service, p);
        boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

        tester::Server s(io_service, port, p);

        std::cout << "# Start tester listening server on " << port << " port.." << std::endl;

        io_service.run();

        t.join();
#endif

    } catch (std::exception& e) {
        std::cout << "# Exception (main): " << e.what() << std::endl;
        error = -100;
    }

close:

    if(error == 0) {
        std::cout << "# Close receiver success.." << std::endl;
    }
    else if(error != 200){
        std::cout << "# Close receiver with error = " << error << std::endl;
    }
    std::cout << "###########################################################" << std::endl;

    return error;


}

#endif
