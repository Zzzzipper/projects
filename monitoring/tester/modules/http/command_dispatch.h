#pragma once

#include <boost/asio/ip/address.hpp>

#include "dispatcher.h"
#include "holder.h"

namespace http {
namespace server {

struct reply;
struct request;

class CommandDispatch  {
public:
    CommandDispatch(const CommandDispatch&) = delete;
    CommandDispatch& operator=(const CommandDispatch&) = delete;

    explicit CommandDispatch();

    void dispatch(const request& req, reply&,
                  boost::asio::ip::address&, std::atomic<bool>&);

};

} // server
} // http
