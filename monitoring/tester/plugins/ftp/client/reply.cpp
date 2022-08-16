#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <istream>


#include "reply.h"

namespace {

using namespace ftp::client;

boost::regex regex_ip(
        ".+\\(([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,})\\).*");

Error DoReadReply(
        boost::asio::ip::tcp::socket &socket,
        boost::asio::streambuf &buffer,
        int& code,
        std::string& message) {
    std::istream stream(&buffer);
    // Response format is '<code> message\r\n'
    boost::system::error_code ec;
    boost::asio::read_until(socket, buffer, "\r\n", ec);
    if (ec) {
        return {ec};
    }
    std::string response;
    ReadLine(stream, response);
    if (response.size() < 3) {
        return {1, "Wrong response: " + response};
    }

    if (response.size() > 3 && response[3] == '-') {
        // Multiline response
        std::istream stream(&buffer);
        std::string code_str = response.substr(0, 3);
        boost::asio::read_until(socket, buffer, boost::regex(code_str + ".+\r\n"), ec);
        if (ec) {
            return Error{ec};
        }
        std::string tail;
        while (ReadLine(stream, tail)) {
            response += tail;
        }
    }

    code = boost::lexical_cast<int>(response.data(), 3);
    message.swap(response);
    return {};
}

} // anonymous namespace

namespace ftp::client {

Error CheckReply(
        boost::asio::ip::tcp::socket &socket,
        boost::asio::streambuf &buffer,
        int expected_code) {

    int code = 0;

    std::string message;

    if (auto err = DoReadReply(socket, buffer, code, message)) {
        return err;
    }

    if (expected_code != 0 && code != expected_code) {

        auto fmt = boost::format("Expected code %1%, got %2%, (%3%)") % expected_code % code % message;
        return {code, fmt.str()};

    } else if (code >= 500) {

        return {code, message};

    }

    return {};
}

Error CheckTransferStatus(
        boost::asio::ip::tcp::socket &socket,
        boost::asio::streambuf &buffer) {
    int code = 0;
    std::string message;
    if (auto err = DoReadReply(socket, buffer, code, message)) {
        return err;
    }
    switch (code) {
    case 125:
    case 150:
        return {};
    }
    auto fmt = boost::format("Expected code 125 or 150, got %1%, (%2%)") % code % message;
    return {code, fmt.str()};
}

Error GetPasvAddress(
        boost::asio::ip::tcp::socket &socket,
        boost::asio::streambuf &buffer,
        Address &address) {
    std::string message;
    int code;
    if (auto err = DoReadReply(socket, buffer, code, message)) {
        return err;
    }
    if (227 != code) {
        auto fmt = boost::format("Expected code 227, got %1%, (%2%)") % code % message;
        return {code, fmt.str()};
    }
    // Get a IP address string.
    address.host = boost::regex_replace(message, regex_ip, "$1.$2.$3.$4", boost::format_all);
    auto itmp1 = boost::lexical_cast<unsigned int>(
                boost::regex_replace(message, regex_ip, "$5", boost::format_all));

    auto itmp2 = boost::lexical_cast<unsigned int>(
                boost::regex_replace(message, regex_ip, "$6", boost::format_all));

    // Get a Port number(16bit).
    address.port = itmp1 * 256 + itmp2;

    return Error{};
}

bool ReadLine(std::istream& stream, std::string& line) {
    if (std::getline(stream, line)) {
        // Discard trailing '\r'
        if (line.back() == '\r') {
            line.pop_back();
        }
        return true;
    }
    return false;
}

} // namespace ftp::client
