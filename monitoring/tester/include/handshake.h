#pragma once

#include <string>
#include <boost/asio.hpp>
#include "protocol.h"

using boost::asio::ip::tcp;

namespace  tester {
class Handshake {
public:
    explicit Handshake();

    /// Get safe socket connection with server
    bool toServer(tcp::socket& socket);

    /// Validation safe socket connection from client
    bool fromClient(tcp::socket& socket, std::vector<uint8_t> &buffer);

private:

    void _initCore();

private:

    std::vector<uint8_t> _ownPrivate;
    std::vector<uint8_t> _ownPublic;
    std::vector<uint8_t> _shared;

    std::array<uint8_t, AES_SECRET_PART_LEN> _key;
    std::array<uint8_t, AES_SECRET_PART_LEN> _ivec;
};
}
