//
// ping.cpp
// ~~~~~~~~
//
//

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>

#include "scope.h"
#include "protocol.h"
#include "iplugin.h"
#include "icmp_header.h"
#include "ipv4_header.h"
#include "log.h"
#include "json.h"
#include "utils.h"
#include "plugin_api.h"

static const char* _tag = "[PING] ";


#if 0

//
// Set log level. Enable output log datetime, level simbol and
// file place swithed in config.h
//
TLogLevel Log::reportingLevel = DEBUG;

/**
 * @brief Main Function
 */
int main( int argc, char* argv[] )
{
    // Parse input
    if( argc < 2 ){
        std::cerr << "usage: " << argv[0] << " <address> <max-attempts = 3>" << std::endl;
        return 1;
    }

    // Get the address
    std::string host = argv[1];

    // Get the max attempts
    int max_attempts = 1;
    if( argc > 2 ){
        max_attempts = str2num<int>(argv[2]);
    }
    if( max_attempts < 1 ){
        std::cerr << "max-attempts must be > 0" << std::endl;
        return 1;
    }

    // Execute the command
    std::string details;
    bool result = Ping( host, max_attempts, details );

    // Print the result
    std::cout << host << " ";
    if( result == true ){
        std::cout << " is responding." << std::endl;
    }
    else{
        std::cout << " is not responding.  Cause: " << details << std::endl;
    }

    return 0;
}
#endif

IPluginApi* pluginApi(PLUGIN_NAME) = nullptr;

namespace ping {

class PingExecutor: public Executor {
public:
    PingExecutor(uint32_t id) : Executor(id) {}
    virtual ~PingExecutor(){}

    /**
     * @brief run - Isolated in owner object method, not static!
     * @return
     */
    virtual int32_t run() {

        // This need for save object data for using
        // after the thread will be detach

        auto savedHostName = _hostName;
        auto result = _result;

        std::string details;
        std::string command = "ping -c " + std::to_string(1) + " " + savedHostName + " 2>&1";

        // Execute the ping command
        result = executeCommand(command, details);

        LOG_TRACE << _tag << savedHostName << " details: " << details << ", result: " << std::dec << result;

        return result;
    }
};

struct PluginApiInitializer {
    PluginApiInitializer() {
        static PluginApiClass<PingExecutor> pluginApi;
        pluginApi(PLUGIN_NAME) = &pluginApi;
    }
};

} // namespace ping

ping::PluginApiInitializer initializer;








