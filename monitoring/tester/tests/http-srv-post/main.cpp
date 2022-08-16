#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
//#include <boost/property_tree/ptree_serialization.hpp>
#include <iostream>
#include <fstream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using net::ip::tcp;
using namespace std::string_literals;
using boost::property_tree::ptree;

static std::string const fname = "upload.txt";

int main() {
    net::io_context io;
    tcp::acceptor a(io, {{}, 8081});

    tcp::socket s(io);
    a.accept(s);

    std::cout << "Receiving request from " << s.remote_endpoint() << "\n";

    http::request<http::string_body> req;
    net::streambuf buf;
    http::read(s, buf, req);

    std::cout << "Method: " << req.method() << "\n";
    std::cout << "URL: " << req.target() << "\n";
    std::cout << "Content-Length: "
              << (req.has_content_length()? "explicit ":"implicit ")
              << req.payload_size() << "\n";
    std::cout << "Content-Type: " << req[http::field::content_type] << "\n";

    std::cout << "Writing " << req.body().size() << " bytes to " << fname << "\n";

    std::cout << "Body " << req.body() << "\n";

    std::stringstream ss, sout;
    ss << req.body();
    ptree pt;
    boost::property_tree::read_json(ss, pt);
    std::cout << "Command is: " << pt.get<std::string>("command") << std::endl;

    boost::property_tree::write_json(ss, pt);
    std::cout << "Serialized Json: " << ss.str() << std::endl;

    std::ofstream(fname) << req.body();

    {
        http::response<http::string_body> response;
        response.reason("File was accepted");
        response.body() = std::move(req.body());
        response.keep_alive(false);
        response.set("XXX-Filename", fname);

        http::write(s, response);
    }
}
