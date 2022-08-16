#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <chrono>
#include <cctype>
#include <regex>

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
#include "processor.h"
#include "record.h"
#include "m_types.h"
#include "watchdog.h"
#include "ver.h"
#include "build.h"
#include "requestor.h"
#include "sender.h"
#include "dispatcher.h"
#include "stamp.h"
#include "holder.h"
#include "timer.h"

#include "server.h"

namespace po = boost::program_options;

static const char* _tag = "[MAIN] ";

/**
 * @brief The Handler class entity. Control collate statistic.
 */
class Handler
{
public:
    Handler(boost::asio::io_service& io, tester::Processor& p)
        : strand_(io),
          timer_(io, boost::posix_time::seconds(TICK_INTERVAL_SEC)),
          p_(p)
    {
        timer_.async_wait(strand_.wrap(boost::bind(&Handler::message, this)));
    }

    void message() {
        try {
            LOG_INFO << _tag << p_.echoStatus().str();
            timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(TICK_INTERVAL_SEC));
            timer_.async_wait(strand_.wrap(boost::bind(&Handler::message, this)));
        } catch(std::exception& e) {
            LOG_ERROR << _tag << "Exception (Handler::message): " << e.what();
        }
    }

private:
    boost::asio::io_service::strand strand_;
    boost::asio::deadline_timer timer_;
    tester::Processor& p_;
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

/**
 * @brief separeateAddress - Separate address from <host:port> in two strings
 * @param addr
 * @param ip_port
 * @return
 */
bool separateAddress(std::string addr, std::vector<std::string>& ip_port) {
    ip_port.clear();
    std::istringstream f(addr);
    std::string s;
    while (getline(f, s, ':')) {
        ip_port.push_back(s);
    }
    if(ip_port.size() != 2) {
        return false;
    }
    if(ip_port[1].size() == 2 &&
            std::count_if(ip_port[1].begin(), ip_port[1].end(),
                          [](unsigned char c){ return std::isdigit(c); }
                          ) == ip_port[1].size())  {
        return false;
    }
    return true;
}

/// Shutdown handlers
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
                ("i", po::value<int>(), "tester ID")
                ("p", po::value<int>(), "testers listening TCP port (default 10001)")
                ("w", po::value<int>(), "http server port (default 8080)")
                ("t", po::value<int>(), "request tick delay, sec. (default 30)")
                ("f", po::value<std::string>(), "local database file (default ./record_container.db)")
                ("h", po::value<std::string>(), "htdocs directory (default ./)")
                ("r", po::value<std::string>(), "receiver address, use format<ip_addr:port>")
                ("n", po::value<std::string>(), "tester nickname (default 'Tester')")
                ("g", po::value<std::string>(), "tester plugin path (default .)")
                ("l", po::value<std::string>(), "tester log path (default .)")
                ("c,config", po::value<std::string>(), "inintializing config file path (default ./config.ini)");


        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            goto close;
        }

        //
        //  Trying to read config from file
        //
        std::string configIniPath = "../config.ini";
        if (vm.count("c")) {
            configIniPath = vm["c"].as<std::string>();
        }

        bool configRead = false;
        auto readSetting = [&](po::options_description& desc, po::variables_map& vm) {
            std::ifstream iniFile(configIniPath);
            std::regex regex("(config).*ini");
            std::smatch match;
            const std::string localFilePath = configIniPath;
            if(!std::regex_search(localFilePath.begin(), localFilePath.end(), match, regex)) {
                LOG_INFO << _tag << "# config ini file "
                         << configIniPath << " is unaccessible..";
                return;
            }
            if(!iniFile.is_open()) {
                return;
            }
            configRead = true;
            vm = po::variables_map();
            po::store(po::parse_config_file(iniFile , desc), vm);
            po::notify(vm);
        };

        readSetting(desc, vm);
        //

        std::string logPath = ".";
        if (vm.count("l")) {
            logPath = vm["l"].as<std::string>();
        }

        FileLogger::instance().enableFileLog(logPath.c_str());

        LOG_INFO << _tag << "# config ini file was set to " << configIniPath;
        LOG_INFO << _tag << "# config ini file "
                 << configIniPath << " is " << ((configRead)?"": "not") << " read";
        LOG_INFO << _tag << "# log path was set to " << logPath;

        std::cout << "###########################################################" << std::endl;
        std::cout << "# Start "  << PROJECT_NAME
                  << ", ver.: "  << PROJECT_VER
                  << ", build: " << BUILD_NUMBER << std::endl;
        std::cout << "#" << std::endl;

        int requestDelay = DEF_REQUEST_PERIOD;
        if (vm.count("t")) {
            requestDelay = vm["t"].as<int>();
        }
        LOG_INFO << _tag << "# request tick delay was set to " << requestDelay;

        int tid;
        auto hasReceiverIdError = false;
        if (vm.count("i")) {
            tid = vm["i"].as<int>();
        } else {
            LOG_ERROR << _tag  << "# Not applyed tester ID, tester will work as http web server..";
            hasReceiverIdError = true;
        }

        if(!hasReceiverIdError) {
            LOG_INFO << _tag << "# receiver ID was set to " << tid;
        }

        std::string pluginPath = ".";
        if (vm.count("g")) {
            pluginPath = vm["g"].as<std::string>();
        }
        LOG_INFO << _tag << "# plugins path was set to " << pluginPath;

        //------------------------------------------------------------
        // Load plugins
        tester::Holder::instance().loadPlugins(pluginPath.c_str());
        tester::Holder::instance().print(LOG_DEBUG);

        //------------------------------------------------------------
        auto hasReceiverAddrError = false;
        std::vector<std::string> receiverAddrPair;

        if(!hasReceiverIdError) {
            std::string addr;
            if (vm.count("r")) {
                addr = vm["r"].as<std::string>();
            } else {
                LOG_ERROR << _tag  << "# Not applyed receiver addrees , tester will work as http web server..";
                hasReceiverAddrError = true;
            }

            if(!hasReceiverAddrError) {
                if(!separateAddress(addr, receiverAddrPair)) {
                    LOG_ERROR << _tag  << "# Error detecting receiver address and port, tester will work as http web server..";
                    hasReceiverAddrError = true;
                }
            }

            if(!hasReceiverAddrError) {
                LOG_INFO << _tag << "# receiver address was set to " << receiverAddrPair[0] << ":" << receiverAddrPair[1];
            }
        }

        std::string testerName = "Tester";
        if (vm.count("n")) {
            testerName = vm["n"].as<std::string>();
        }
        LOG_INFO << _tag << "# tester nickname was set " << testerName;


        int port = 10001;
        if (vm.count("p")) {
            port = vm["p"].as<int>();
        }
        LOG_INFO  << _tag << "# testers listening TCP port was set to " << port;

        std::string dbpath("./record_container.db");
        if (vm.count("f")) {
            dbpath = vm["f"].as<std::string>();
        }
        LOG_INFO << _tag << "# local database file was set to " << dbpath;

        std::string htdocs("./");
        if (vm.count("h")) {
            htdocs = vm["h"].as<std::string>();
        }
        LOG_INFO << _tag << "# htdocs directory was set to " << htdocs;

        //------------------------------------------------------------
        // Module thread array
        std::vector<boost::thread> many;
        std::atomic<bool> running(true);

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

        int httpPort = 8080;
        if (vm.count("w")) {
            try {
                httpPort = vm["w"].as<int>();
            } catch(std::exception &e) {
                LOG_ERROR << _tag << e.what();
            }
        }

        while(port_in_use(httpPort)) {
            httpPort = httpPort + 10;
        }

        LOG_INFO << _tag << "# http server port was set to " << httpPort;

        // Set tester id
        tester::Dispatcher::instance().setId(tid);
        // Set tester nickname
        tester::Dispatcher::instance().setName(testerName);
        // Set tester version value
        auto strVersion = std::string(PROJECT_VER)
                + "." + std::to_string(BUILD_NUMBER);
        tester::Dispatcher::instance().setVersion(strVersion);

        // Set tester info
        // Tester v2.14(HRT)(MD)/w64, Compiled:Apr 12 2011 22:20:51, Source:Tester.cpp,Tue Apr 12 22:20:46 2011
        std::stringstream stringStream;
        stringStream << testerName
                     << " v" << strVersion
                     << ", Compiled: " << COMPILE_TIME
                     << ", Git commit: " << GIT_COMMIT
                     << ", Time: " << GIT_COMMIT_TIME;

        tester::Dispatcher::instance().setInfo(stringStream.str());

        http::server::Server server("0.0.0.0", std::to_string(httpPort), htdocs, running);

        //------------------------------------------------------------
        // Catch SIGINT
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
                    LOG_INFO << _tag << "Start HTTP server module..";
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
        });

        if(!hasReceiverIdError && !hasReceiverAddrError) {
            //------------------------------------------------------------
            // Requestor loop
            //
            many.emplace_back([&running, &receiverAddrPair, &tid, &requestDelay, &t] {

                tester::Requestor requestor(tid);
                t.setObserver(&requestor);

                while(running) {
                    if(requestor.isReady(requestDelay)) {
                        try {
                            tester::Dispatcher::instance().clearRequests();
                            LOG_INFO << _tag << "Start request configs from receiver..";
                            if(requestor.requestTasks(receiverAddrPair[0].c_str(), receiverAddrPair[1].c_str())) {
                                if(tester::Dispatcher::instance().getExtConfig().CfgRereadPeriod > 0) {
                                    requestDelay = tester::Dispatcher::instance().getExtConfig().CfgRereadPeriod;
                                    LOG_WARNING << _tag << "Request delay was set to " << requestDelay;
                                    LOG_INFO << _tag << "Start work with task..";
                                    tester::Dispatcher::instance().executeTasks();
                                }
                            } else {
                                requestDelay = DEF_REQUEST_PERIOD;
                            }
                            if(!running) {
                                return;
                            }
                        } catch(std::runtime_error e) {
                            LOG_ERROR << _tag << "Requestor get tasks failed: exception " << e.what() << "..";
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(TICK_INTERVAL_SEC));
                }
                LOG_INFO << _tag << "Config requestor stopped..";
            });

            //------------------------------------------------------------
            // Sender loop
            //
            many.emplace_back([&running, &receiverAddrPair, &tid, &t] {

                tester::Sender sender(tid);
                t.setObserver(&sender);

                while(running) {
                    if(sender.isReady(UPLOAD_PERIOD)) {
                        try {
                            LOG_INFO << _tag << "Start upload result to receiver..";
                            // Initialise the requestor.
                            sender.uploadResults(receiverAddrPair[0].c_str(), receiverAddrPair[1].c_str());
                            if(!running) {
                                return;
                            }
                        } catch(std::exception e) {
                            LOG_ERROR << _tag << "Exception " << e.what();
                            LOG_ERROR << _tag << "Sender send results failed..";
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(TICK_INTERVAL_SEC));
                }
                LOG_INFO << _tag << "Result sender stopped..";
            });
        }

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
        LOG_ERROR << "Exception (main): " << e.what();
        error = -100;
    }

close:

    if(error == 0) {
        std::cout << "# Close tester success.." << std::endl;
    } else {
        std::cout << "# Close tester with error = " << std::endl;
    }
    std::cout << "###########################################################" << std::endl;

    return error;


}

#endif
