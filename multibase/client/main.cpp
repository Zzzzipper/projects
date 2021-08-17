#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

using namespace boost::program_options;
using namespace std;

#include "config.h"
#include "json.h"
#include "m_types.h"


using boost::asio::ip::tcp;

namespace po = boost::program_options;

//
// Set log level. Enable output log datetime, level simbol and
// file place swithed in config.h
//
TLogLevel Log::reportingLevel = DEBUG;

/**
 * @brief preCheckArgs - previous check arguments
 * @param vm
 * @return
 */
bool preCheckArgs(variables_map &vm) {

    if(!vm.count("operation")) {
        LOG_ERROR << "Not set operation, exit..";
        return false;
    } else {
        auto command = boost::algorithm::to_lower_copy(vm["operation"].as<string>());
        if(command == "delete" || command == "get") {
            if(!vm.count("key")) {
                LOG_ERROR << "Not set key for delete op, exit..";
                return false;
            }
        } else {
            if(!vm.count("key") || !vm.count("value")) {
                LOG_ERROR << "Not set valid key pair, exit..";
                return false;
            }
        }
    }

    return true;
}

void test(const std::string host_, const std::string port_);

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */

#if 1
int main(int argc, char* argv[]) {

    if(argc == 1) {
        LOG_ERROR << "No args..";
        return 0;
    }

    string operation, key, value_, host, port;

    options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "print usage message")
            ("addr,a", value(&host), "multibase server host")
            ("port,p", value(&port), "multibase server port")
            ("operation,o", value(&operation), "operation: INSERT, UPDATE, DELETE or GET")
            ("key,k", value(&key), "key of value")
            ("value,v", value(&value_), "value body")
            ("test,t", "test mode")
            ;

    try {
        variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);

        // Show help and return
        if (vm.count("help")) {
            cout << desc << endl;
            return multibase::ArgsError;
        }

        // Run in test mode
        if(vm.count("test")) {
            if(vm.count("addr") && vm.count("port")) {
                test(vm["addr"].as<string>(), vm["port"].as<string>());
            } else {
                return multibase::ArgsError;
            }
        }

        if(!preCheckArgs(vm)) {
            return multibase::ArgsError;
        }

        LOG_TRACE << "Success args..";

        boost::asio::io_service io_service;

        tcp::socket s(io_service);
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(vm["addr"].as<string>(), vm["port"].as<string>());
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        boost::system::error_code ec;
        boost::asio::connect(s, endpoint_iterator);

        value_ = (vm["value"].empty())? ".." : vm["value"].as<string>();

        multibase::request r = {
            vm["operation"].as<string>(),
            vm["key"].as<string>(),
            value_
        };

        // Serialize command to string buffer aka simple packet
        std::ostringstream put, get;
        json::serializer< multibase::request >::serialize(put, r);
        std::string buffer = put.str();

        LOG_DEBUG << "Request: " << buffer;
        boost::asio::write(s, boost::asio::buffer(buffer, buffer.length()));

        for (;;)  {
            boost::array<char, 128> buf;
            boost::system::error_code error;

            size_t len = s.read_some(boost::asio::buffer(buf), error);
            if (error) {
                throw boost::system::system_error(error); // Some other error.
            }

            if(len > 0) {
                get << buf.data();
            }

            if (error != boost::asio::error::would_block) {
                break; // Connection closed cleanly by peer.
            }
        }

        std::string replayBuffer = get.str();

        using boost::spirit::ascii::space;
        typedef std::string::const_iterator iterator_type;
        typedef response_parser<iterator_type> response_parser;
        response_parser g; // Our grammar

        multibase::response response;
        std::string::const_iterator iter = replayBuffer.begin();
        std::string::const_iterator end = replayBuffer.end();
        bool result = phrase_parse(iter, end, g, space, response);

        // TODO: bug. The parsing is sussess but return is a false ??
        // The response complete sent from server all time, not another format.
        //        if(result) {
        LOG_INFO << "Reply code: " << response.code
                 << ", what: " << response.what;
        return response.code;
        //        }

    } catch (exception& e) {
        LOG_ERROR << "Exception (main): " << e.what();
    }

    return multibase::AppError;
}

#endif


