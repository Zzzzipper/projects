#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "crypto.h"
#include "common.h"
#include "oke.h"
#include "hex_dump.h"


using boost::asio::ip::tcp;

const int max_length = 1024;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

void session(socket_ptr sock)
{
  try
  {
    for (;;)
    {
      char data[max_length];

      boost::system::error_code error;
      size_t length = sock->read_some(boost::asio::buffer(data), error);

      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.

      std::string clientKey = std::string(data, length);

      std::cout << ">>>>>Receive public key from client:" << std::endl;
      std::cout << "  ECDH-PUB-KEY:" << std::endl;
      dash::hex_dump(clientKey);

      crypto::ownkey_s  server_key;

      if (!crypto::generate_ecdh_keys(server_key.ec_pub_key, server_key.ec_priv_key))
      {
          std::cout << "ECDH-KEY generation failed." << std::endl;
          return;
      }

      boost::asio::write(*sock, boost::asio::buffer(server_key.ec_pub_key,
                                                    sizeof(server_key.ec_pub_key)));
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception in thread: " << e.what() << "\n";
  }
}

void server(boost::asio::io_service& io_service, short port)
{
  tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
  for (;;)
  {
    socket_ptr sock(new tcp::socket(io_service));
    a.accept(*sock);
    boost::thread t(boost::bind(session, sock));
  }
}

int main(int argc, char* argv[])
{
  try
  {

    boost::asio::io_service io_service;

    using namespace std; // For atoi.
    server(io_service, 10001);
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
