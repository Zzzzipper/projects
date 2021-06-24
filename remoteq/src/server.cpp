#include <algorithm>
#include <string>
#include <memory>

#include "common.h"
#include "udsserver.h"
#include "loguru.h"
#include "headmeters.h"

// Declare main handler of server
uds_command_t* request_handler(uds_command_t *req);

/**
 * @brief getCmdOption - Return string option to next after -<simbol> option
 * @param begin
 * @param end
 * @param option
 * @return
 */
char* getCmdOption(char ** begin, char ** end, const std::string & option) {
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return nullptr;
}

/**
 * @brief cmdOptionExists - check than option presents in args
 * @param begin
 * @param end
 * @param option
 * @return
 */
bool cmdOptionExists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

/**
 * @brief install_sig_handler
 */
void install_sig_handler()
{
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    act.sa_handler = uds::UdsServer::handler_sigint;
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);
}

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[])
{
    // logger setup

    loguru::init(argc, argv);

    loguru::Verbosity v = loguru::Verbosity_MAX;
    std::string path = UDS_SOCK_PATH;

    char *logfile   = getCmdOption(argv, argv + argc, "-f");
    char *verbosity = getCmdOption(argv, argv + argc, "-v");
    char *sock_path = getCmdOption(argv, argv + argc, "-s");

    // Get socket path if present
    if(sock_path) {
        path = sock_path;
    }

    if (logfile) {
        if(verbosity) {
            v = std::atoi(verbosity);
        }
        loguru::add_file(logfile, loguru::Append, v);
    } else {
        loguru::add_file("server.log", loguru::Append, v);
    }

    LOG_F(INFO, "HeadMeter UDS server starting.. ");

    std::unique_ptr<uds::UdsServer> server(uds::UdsServer::instance(path.c_str(), &request_handler));
    if(!server->create()) {
        LOG_F(ERROR, "server: init error");
    }

    // The firstly install signal handler
    install_sig_handler();

    // Create headmeters
    std::unique_ptr<HeadMeters> measurement(HeadMeters::instance());

    // Infiniti server loop: while _loop_flaf is 1
    server->exec();

    return STATUS_SUCCESS;
}
