#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/process/system.hpp>
#include <iostream>

#include "command_dispatch.h"
#include "reply.h"
#include "request.h"
#include "stamp.h"
#include "log.h"
#include "utils.h"
#include "dispatcher.h"

using boost::property_tree::ptree;
using boost::process::system;

static const char* _tag = "[HTTP DISP] ";

namespace http {
namespace server {

/**
 * @brief CommandDispatch::CommandDispatch
 */
CommandDispatch::CommandDispatch()
{}

/**
 * @brief CommandDispatch::dispatch
 * @param request      - reference to request object struct
 * @param reply        - reference to reply object struct
 * @param localAddress - resolved host IPv4 address
 * @param running      - running valve flag
 */
void CommandDispatch::dispatch(const request &req, reply &reply,
                               boost::asio::ip::address& localAddress, std::atomic<bool>& running) {

    //
    // Parse ptree
    //
    std::string command;

    if(req.method == "POST") {
        if(req.body.size() != 0) {
            std::stringstream ss;
            ss << req.body.data() << "\x0";
            ptree pt;
            boost::property_tree::read_json(ss, pt);

            command = pt.get<std::string>("command");
            if(command.empty()) {
                reply = reply::stock_reply(reply::not_found);
                return;
            }
        }
    } else if(req.method == "GET") {
        command = getParametersFromUri(req.uri);
    }

    LOG_INFO << _tag << "Command is: " << command;

    std::stringstream out;
    std::string buffer;

    if(command == "getStatus") {
        out << "{";
        out << "\"""success\""": true,";
        out << "\"tester\": {";
//        out << "\"id\":" << dispatcher.getId() << ",";
//        out << "\"testerid\": " << dispatcher.getId() << ",";
        out << "\"clientid\": null,";
        out << "\"active\": true,";
//        out << "\"testername\": \"" << dispatcher.getName() << "\",";
        out << "\"lastactivity\": null,";
        out << "\"tasksnumber\": 0,";
//        out << "\"softversion\": \"v" + dispatcher.getVersion() + "\",";
        out << "\"ip\": \"" << localAddress.to_string() << "\",";
        out << "\"port\": \"--\",";
        out << "\"cancheck\": true,";
        out << "\"pass\": \"test\",";
//        out << "\"totaltests\": \"" + std::to_string(dispatcher.size()) +"\",";
        out << "\"totalchecks\": 0,";
        out << "\"cfgrereadperiod\": 1,";
        out << "\"bodmaxage\": 2,";
        out << "\"chgmaxage\": 3,";
        out << "\"isonline\": false,";
//        out << "\"comment\": \"" << dispatcher.getInfo() << "\",";
        out << "\"created_at\": \"" << COMPILE_TIME << "\",";
        out << "\"updated_at\": \"" << GIT_COMMIT_TIME << "\",";
        out << "\"modules\": [1, 3, 13],";
//        out << "\"loads\":" << holder.getTasksByPlugin();
        out << "}";
        out << "}";
    }
    else if(command == "reload") {
#if 0
        running = false;
        kill(0, SIGINT);
        out << "{\"""ok\""": \"""reloaded\"""}";
#else
        auto result = system("echo $(cat ~/.credentials) |  sudo -S systemctl restart tester.service");
        out << "{\"""ok\""": \"""reloaded\""", \"""result\""": "<< result << "}";
        LOG_TRACE << _tag << "Reload result " << result;
#endif
    }
    // GET command
    else if(command == "status") {
        out << "{";
        out << "\"""success\""": true,";
        out << "\"receiver\": {";
        out << "\"active\": true,";
        out << "\"ip\": \"" << localAddress.to_string() << "\",";
        out << "\"port\": \"--\",";
        out << "\"cancheck\": true,";
        out << "\"created_at\": \"" << COMPILE_TIME << "\",";
        out << "\"updated_at\": \"" << GIT_COMMIT_TIME << "\"";
        out << "}";
        out << "}";
    }
    else {
        out << "{\"""error\""": \"""unknown method\"""}";
    }

    buffer = out.str();

    LOG_TRACE << _tag << buffer;

    // Fill out the reply to be sent to the client.
    reply.status = reply::ok;
    reply.content.append(buffer.data(), buffer.size());
    reply.headers.push_back({"Content-Type", "application/json; charset=utf-8"});
    reply.headers.push_back({"Content-Length", std::to_string(buffer.size())});
    reply.headers.push_back({"Content-Length", std::to_string(buffer.size())});

    return;
}
}
}
