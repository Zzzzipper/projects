//
// RequestHandler.hpp
//

#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP

#include <string>
#include <boost/asio/ip/address.hpp>

namespace http {
namespace server {

struct reply;
struct request;

/// The common handler for all incoming requests.
class RequestHandler
{
public:
  RequestHandler(const RequestHandler&) = delete;
  RequestHandler& operator=(const RequestHandler&) = delete;

  /// Construct with a directory containing files to be served.
  explicit RequestHandler(const std::string&, std::atomic<bool>&);

  /// Handle a request and produce a reply.
  void handle_request(const request&, reply&);

  /// Set local endpoint address
  void setLocalEndPoint(boost::asio::ip::address addr);

private:
  /// The directory containing the files to be served.
  std::string _doc_root;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);

  /// Local socket address
  boost::asio::ip::address _local_addr;

  /// Running valve
  std::atomic<bool>& _running;
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HANDLER_HPP
