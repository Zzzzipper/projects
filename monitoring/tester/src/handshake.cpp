#include "handshake.h"
#include "aes.h"
#include "log.h"
#include "hex_dump.h"

extern "C" void curve25519_donna(uint8_t *, const uint8_t *, const uint8_t *);

static const char* _tag  = "[CLI] ";
static const char* _pass = "It's a fracking tester's dAta";

static const uint8_t _basepoint[32] = {9};

namespace tester {

/**
 * @brief Handshake::Handshake
 * @param host
 * @param port
 */
Handshake::Handshake()
    : _ownPrivate(DH_PUBLIC_KEY_LENGTH, 0),
      _ownPublic(DH_PUBLIC_KEY_LENGTH, 0),
      _shared(DH_PUBLIC_KEY_LENGTH, 0)
{
    _initCore();
}

/**
 * @brief Handshake::_initCore
 */
void Handshake::_initCore() {
    std::srand(std::time(nullptr));
    auto value = (unsigned int)
            (254*(1.0*(unsigned int)std::rand()/
                  ((unsigned int)RAND_MAX+1)));
    std::fill(_ownPrivate.begin(), _ownPrivate.end(), value);
    LOG_TRACE << _tag << "Fill private: " << value;
    value = (unsigned int)
            (254*(1.0*(unsigned int)std::rand()/
                  ((unsigned int)RAND_MAX+1)));
    _ownPrivate[0] &= value;
    LOG_TRACE << _tag << "0: " << value;
    value = (unsigned int)
            (254*(1.0*(unsigned int)std::rand()/
                  ((unsigned int)RAND_MAX+1)));
    _ownPrivate[31] &= value;
    LOG_TRACE << _tag << "31: " << value;
    value = (unsigned int)
            (254*(1.0*(unsigned int)std::rand()/
                  ((unsigned int)RAND_MAX+1)));
    _ownPrivate[31] |= value;
    LOG_TRACE << _tag << "31: " << value;

    curve25519_donna(_ownPublic.data(), _ownPrivate.data(), _basepoint);
}

/**
 * @brief Get safe socket connection
 * @return
 */
bool Handshake::toServer(tcp::socket& socket) {

    auto result = false;

    try {
        boost::asio::write(socket, boost::asio::buffer(_ownPublic));

        std::array<uint8_t, DH_PUBLIC_KEY_LENGTH> reply;

        boost::asio::read(socket, boost::asio::buffer(reply));

        LOG_TRACE << _tag << "Start exchange with receiver...";
        dash::hex_dump(std::string(reinterpret_cast<char*>(reply.data()),
                                   DH_PUBLIC_KEY_LENGTH), LOG_TRACE << _tag);

        LOG_TRACE << _tag << "Calculate shared secret ";
        curve25519_donna(_shared.data(), _ownPrivate.data(), reply.data());

        std::copy(_shared.begin(), _shared.begin() + AES_SECRET_PART_LEN, _key.data());
        std::copy(_shared.begin() + AES_SECRET_PART_LEN, _shared.end(), _ivec.data());

        dash::hex_dump(std::string(reinterpret_cast<char*>(_shared.data()),
                                   DH_PUBLIC_KEY_LENGTH), LOG_TRACE << _tag);

        LOG_TRACE << _tag << "Send session pass.. ";

        SESSION_SUBKEY sessionKey;
        memset(sessionKey.garbage, 25, sizeof(sessionKey.garbage));
        memset(sessionKey.pass, 0, sizeof(sessionKey.pass));
        memcpy(sessionKey.pass, _pass, strlen(_pass));

        std::vector<uint8_t> encrypted(sizeof(SESSION_SUBKEY));

        Encrypt((uint8_t*)&sessionKey, encrypted.data(), sizeof(SESSION_SUBKEY), _key.data(), _ivec.data());

        boost::asio::write(socket, boost::asio::buffer(encrypted));

        result = true;

    }  catch (std::runtime_error e) {
        LOG_ERROR << _tag << "Handshake has error: exception " << e.what() << "..";
    }

    return result;

}

/**
 * @brief Validation safe socket connection from client
 * @return
 */
bool Handshake::fromClient(tcp::socket& socket, std::vector<uint8_t>& buffer) {

    auto result = false;

    try {

        LOG_TRACE << _tag << "Start exchange with receiver...";
        dash::hex_dump(std::string(reinterpret_cast<char*>(buffer.data()),
                                   DH_PUBLIC_KEY_LENGTH), LOG_TRACE << _tag);

        LOG_TRACE << _tag << "Calculate shared secret ";

        curve25519_donna(_shared.data(), _ownPrivate.data(), buffer.data());

        std::copy(_shared.begin(), _shared.begin() + AES_SECRET_PART_LEN, _key.data());
        std::copy(_shared.begin() + AES_SECRET_PART_LEN, _shared.end(), _ivec.data());

        dash::hex_dump(std::string(reinterpret_cast<char*>(_shared.data()),
                                   DH_PUBLIC_KEY_LENGTH), LOG_TRACE << _tag);

        boost::asio::write(socket, boost::asio::buffer(_ownPublic));

        std::array<uint8_t, sizeof(SESSION_SUBKEY)> reply;
        SESSION_SUBKEY sessionKey;

        boost::asio::read(socket, boost::asio::buffer(reply));

        Decrypt(reply.data(), (uint8_t*)&sessionKey, sizeof(SESSION_SUBKEY), _key.data(), _ivec.data());

        LOG_TRACE << _tag << "Success: " << sessionKey.pass;

        result = (strcmp(sessionKey.pass, _pass) == 0);

    }  catch (std::runtime_error e) {
        LOG_ERROR << _tag << "Handshake has error: exception " << e.what() << "..";
    }

    return result;

}

}
