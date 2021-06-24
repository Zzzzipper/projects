#ifndef __udsserver_h__
#define __udsserver_h__

#include <unistd.h>
#include <signal.h>
#include <string>
#include "common.h"
#include "uds.h"

namespace uds {
class UdsServer {

public:
    // Signal handler
    static void handler_sigint(int);
    static UdsServer* instance(const char *path_, request_handler_t handler);
    ~UdsServer();
    // Create socket and listen server
    bool create();
    // Main loop function
    void exec();

private:
    explicit UdsServer(const char* path_, request_handler_t handler);
    static UdsServer*            _instanse;
    // Flag operate main server accept and process
    // connection cicrle
    static volatile sig_atomic_t _loop_flag;

private:
    request_handler_t _handler = nullptr;
    std::string    _path       = UDS_SOCK_PATH;
    uds_server_t*  _s          = nullptr;
};

};

#endif // __udsserver_h__
