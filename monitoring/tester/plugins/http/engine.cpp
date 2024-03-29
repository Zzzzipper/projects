//
// http_client.cpp
// ~~~~~~~~~~~~~~~
//

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>

#include "log.h"

using boost::asio::ip::tcp;

int httpSend(const char* addr, const char* command)
{
    boost::asio::io_context io_context;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve(addr, "http");

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_context);
    boost::asio::connect(socket, endpoints);

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << "GET " << command << " HTTP/1.0\r\n";
    request_stream << "Host: " << addr << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Send the request.
    boost::asio::write(socket, request);

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (!response_stream || http_version.substr(0, 5) != "HTTP/")
    {
        LOG_ERROR << "Invalid response";
        return 1;
    }
    if (status_code != 200)
    {
        LOG_ERROR << "Response returned with status code " << status_code;
        return 1;
    }

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    // Process the response headers.
    std::string header;
    while (std::getline(response_stream, header) && header != "\r")
        LOG_DEBUG << header;

    // Write whatever content we already have to output.
    if (response.size() > 0)
        LOG_DEBUG << &response;

    // Read until EOF, writing data to output as we go.
    boost::system::error_code error;
    while (boost::asio::read(socket, response,
                             boost::asio::transfer_at_least(1), error))
        LOG_DEBUG << &response;
    if (error != boost::asio::error::eof)
        throw boost::system::system_error(error);

    return 0;
}

#if 0

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cout << "Usage: sync_client <server> <path>\n";
            std::cout << "Example:\n";
            std::cout << "  sync_client www.boost.org /LICENSE_1_0.txt\n";
            return 1;
        }

        return httpSend(argv[1], argv[2]);
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return 0;
}

#endif
