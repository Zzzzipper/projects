#include <unistd.h>
#include <signal.h>
#include "common.h"
#include "loguru.h"
#include "udsserver.h"

namespace uds {

UdsServer* UdsServer::_instanse = nullptr;
volatile sig_atomic_t UdsServer::_loop_flag = 1;

/**
 * @brief UdsServer::UdsServer
 * @param path_     - path to unix file socket
 * @param handler   - main handler of messages
 */
UdsServer::UdsServer(const char* path_, request_handler_t handler)
    : _handler(handler)
    , _path(path_)
{
}

/**
 * @brief UdsServer::~UdsServer
 */
UdsServer::~UdsServer() {
    if(_s) {
        server_close(_s);
    }
}

/**
 * @brief UdsServer::create
 * @return
 */
bool UdsServer::create() {
    _s = server_init(_path.c_str(), _handler);
    return _s != nullptr;
}


UdsServer* UdsServer::instance(const char* path_, request_handler_t handler) {
    if(!_instanse) {
        _instanse = new UdsServer(path_, handler);
    }
    return _instanse;
}

/**
 * @brief UdsServer::handler_sigint
 */
void UdsServer::handler_sigint(int) {
    _loop_flag = 0;
}

/**
 * @brief UdsServer::exec
 * @return
 */
void UdsServer::exec() {
    LOG_F(INFO, "HeadMeter UDS server started success.. ");
    while(_loop_flag && _s) {
        server_accept_request(_s);
    }
}



}



