//
// ConnectionManager.cpp
//

#include "connection_manager.h"

namespace http {
namespace server {

ConnectionManager::ConnectionManager()
{
}

void ConnectionManager::start(connection_ptr c)
{
    _connections.insert(c);
    c->start();
}

void ConnectionManager::stop(connection_ptr c)
{
    _connections.erase(c);
    c->stop();
}

void ConnectionManager::stop_all()
{
    for (auto c: _connections)
        c->stop();
    _connections.clear();
}

} // namespace server
} // namespace http
