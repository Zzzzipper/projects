//
// RequestHandler.cpp
//

#include "request_handler.h"
#include "command_dispatch.h"
#include <fstream>
#include <sstream>
#include <string>
#include "mime_types.h"
#include "reply.h"
#include "request.h"

static const char* _tag = "[HTT REQ HANDLER] ";

namespace http {
namespace server {

RequestHandler::RequestHandler(const std::string& docRoot, std::atomic<bool>& running)
    : _doc_root(docRoot),
      _running(running)
{
}

void RequestHandler::handle_request(const request& request, reply& reply)
{
    // Decode url to path.
    std::string request_path;
    if (!url_decode(request.uri, request_path))
    {
        reply = reply::stock_reply(reply::bad_request);
        return;
    }

    // Request path must be absolute and not contain "..".
    if (request_path.empty() || request_path[0] != '/'
            || request_path.find("..") != std::string::npos)
    {
        reply = reply::stock_reply(reply::bad_request);
        return;
    }

    LOG_TRACE << _tag << "Method: " << request.method;
    LOG_TRACE << _tag << "Uri: " << request.uri;
    LOG_TRACE << _tag << "Minor version: " << request.http_version_minor;
    LOG_TRACE << _tag << "Major version: " << request.http_version_major;
    for(auto &h: request.headers) {
        LOG_TRACE << _tag << "Header: " << h.name << ", Value: " << h.value;
    }
    LOG_TRACE << _tag << "Body: " << request.body.data();

    for(auto iit = request.headers.begin(); iit != request.headers.end(); iit++) {
        if(iit->name == "Origin") {
            reply.headers.push_back({"Access-Control-Allow-Origin", iit->value});
            LOG_TRACE << _tag << "CORS enabled";
            break;
        }
    }

    reply.headers.push_back({"Access-Control-Allow-Methods", "POST, GET, OPTIONS"});
    reply.headers.push_back({"Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type"});
    reply.headers.push_back({"Access-Control-Allow-Credentials", "true"});

    // Dispatch requests to their parser
    if(request.method == "POST" || request.method == "GET") {
        CommandDispatch commandDispatch;
        commandDispatch.dispatch(request, reply, _local_addr, _running);
        return;
    } else if(request.method == "OPTIONS") {
        reply.status = reply::ok;
        return;
    }

    // If path ends in slash (i.e. is a directory) then add "index.html".
    if (request_path[request_path.size() - 1] == '/')
    {
        request_path += "index.html";
    }

    // Determine the file extension.
    std::size_t last_slash_pos = request_path.find_last_of("/");
    std::size_t last_dot_pos = request_path.find_last_of(".");
    std::string extension;
    if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
    {
        extension = request_path.substr(last_dot_pos + 1);
    }

    // Open the file to send back.
    std::string full_path = _doc_root + request_path;
    std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
    if (!is)
    {
        reply = reply::stock_reply(reply::not_found);
        return;
    }

    // Fill out the reply to be sent to the client.
    reply.status = reply::ok;
    char buf[512];
    while (is.read(buf, sizeof(buf)).gcount() > 0)
        reply.content.append(buf, is.gcount());
    reply.headers.resize(2);
    reply.headers[0].name = "Content-Length";
    reply.headers[0].value = std::to_string(reply.content.size());
    reply.headers[1].name = "Content-Type";
    reply.headers[1].value = mime_types::extension_to_type(extension);
}

bool RequestHandler::url_decode(const std::string& in, std::string& out)
{
    out.clear();
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%') {
            if (i + 3 <= in.size()) {
                int value = 0;
                std::istringstream is(in.substr(i + 1, 2));
                if (is >> std::hex >> value) {
                    out += static_cast<char>(value);
                    i += 2;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        else if (in[i] == '+') {
            out += ' ';
        }
        else {
            out += in[i];
        }
    }
    return true;
}

void RequestHandler::setLocalEndPoint(boost::asio::ip::address addr) {
    _local_addr = addr;
}

} // namespace server
} // namespace http
