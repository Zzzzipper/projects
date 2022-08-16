#include "sender.h"
#include "handshake.h"
#include "crc32.h"
#include "hex_dump.h"
#include "log.h"

static const char* _tag = "[SEND] ";

namespace tester {

/**
 * @brief Sender::Sender
 */
Sender::Sender()
    :_tid(0),
      _tickCounter(0)
{}

/**
 * @brief Sender::Sender
 * @param tid - tester index
 */
Sender::Sender(uint32_t tid)
    :_tid(tid),
      _tickCounter(0)
{}

/**
 * @brief Sender::uploadResults - Upload results
 * @param host - IP address or dns host name
 * @param port - TCP port of subject
 */
void Sender::uploadResults(const char *host, const char *port) {
    LOG_TRACE << _tag << "Start uploadResults";

#ifndef RECEIVER_SIDE
    std::vector<uint8_t> out = Dispatcher::instance().outputPack();
    if(out.size() == 0) {
        LOG_INFO << _tag << "Nothing to send..";
        return;
    }

    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    tcp::socket socket(io_service);
    tcp::resolver::query query(tcp::v4(), host, port);
    tcp::resolver::iterator iterator = resolver.resolve(query);

    boost::asio::connect(socket, iterator);

    Handshake handshake;

    if(!handshake.toServer(socket)) {
        socket.close();
        LOG_ERROR << _tag <<"Handshake did not success";
        return;
    }

    auto dataLen = sizeof(ST_SIZES) + sizeof(ST_T_HDR) + out.size();

    const uint32_t size = sizeof(ST_T_HDR) + out.size();

    ST_SIZES sizes = {0, size, 0};
    ST_T_HDR hdr = {_tid, MODULE_PING};

    std::vector<uint8_t> buffer(dataLen);

    memcpy(buffer.data(), &sizes, sizeof(ST_SIZES));
    memcpy(buffer.data() + sizeof(ST_SIZES), &hdr, sizeof(ST_T_HDR));
    memcpy(buffer.data() + sizeof(ST_SIZES) + sizeof(ST_T_HDR), out.data(), out.size());

    uint32_t table[256];
    crc32::generate_table(table);

    uint32_t crc = crc32::update(table, 0xFFFFFFFF, buffer.data(), dataLen);

    LOG_TRACE << _tag << "Request CFG ";
    dash::hex_dump(std::string(reinterpret_cast<char*>(buffer.data()), dataLen),
                   LOG_TRACE << _tag);

    memcpy(buffer.data() + sizeof(uint32_t)*2, &crc, sizeof(uint32_t));

    LOG_TRACE << _tag << "Upload results.. ";
    dash::hex_dump(std::string(reinterpret_cast<char*>(buffer.data()), dataLen),
                   LOG_TRACE << _tag);

    boost::asio::write(socket, boost::asio::buffer(buffer, dataLen));

    auto messageSize = 64;
    std::vector<uint8_t> dataBuffer;

    boost::system::error_code ec;

    while (ec != boost::asio::error::eof) {

        std::vector<char> socketBuffer(messageSize);

        size_t received = socket.read_some(
                    boost::asio::buffer(socketBuffer),ec);

        LOG_TRACE << _tag << "...sender received " << received;

        if (received > 0) {
            LOG_TRACE << _tag << "Received from receiver part " << std::dec << received;
            dash::hex_dump(std::string(socketBuffer.data(), socketBuffer.size()),
                           LOG_TRACE << _tag);
            dataBuffer.insert(dataBuffer.end(),
                              socketBuffer.begin(), socketBuffer.begin() + received);
        }
    }

    if(ec && !boost::asio::error::eof){
        LOG_ERROR << _tag <<"Sender has error: "<< ec.message();
    }

    if(dataBuffer.size() != 0) {
        if(dataBuffer.size() == 4) {
            int32_t response;
            memcpy(&response, dataBuffer.data(), sizeof(int32_t));

            if(response >= 0) {
                LOG_ERROR << _tag << "Receiver accepted "  << std::dec << response << " records";
            }
            else {
                LOG_TRACE << _tag << "Receiver has error: " << rc_error(response);
            }

        } else {
            LOG_DEBUG << _tag << "Sender received " << std::dec << dataBuffer.size() << " bytes..";
        }

        dash::hex_dump(std::string(reinterpret_cast<char*>(dataBuffer.data()), dataBuffer.size()),
                       LOG_TRACE << _tag);

    }
#endif
}

/**
 * @brief tickTimer - Have tick from global timer for synchronizing internal module loop
 */
void Sender::tickTimer() {
    ++_tickCounter;
}

/**
 * @brief isReady - Return readynest of to be start loop iteration
 * @param level
 * @return
 */
bool Sender::isReady(uint32_t level) {
    if(_tickCounter == level) {
        _tickCounter = 0;
        return true;
    }
    return false;

}


}
