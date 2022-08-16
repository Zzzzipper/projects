#include <cstdint>
#include <iostream>

#include <boost/asio.hpp>

#include "hex_dump.h"
#include "requestor.h"
#include "handshake.h"
#include "crc32.h"
#include "log.h"
#include "zlib.h"

extern "C" void curve25519_donna(uint8_t *, const uint8_t *, const uint8_t *);

static const char* _tag = "[REQ] ";

using boost::asio::ip::tcp;

namespace tester {

/**
 * @brief Requestor::Requestor
 */
Requestor::Requestor()
    :_tid(0),
      _tickCounter(0)
{}


/**
 * @brief Requestor::Requestor
 * @param tid - tester id
 */
Requestor::Requestor(uint32_t tid)
    :_tid(tid),
      _tickCounter(0)
{}

/**
 * @brief Requestor::requestTasks - Request and dispatch task from receiver/tester
 * @param host - IP address or dns host name
 * @param port - TCP port of subject
 */
bool Requestor::requestTasks(const char* host, const char* port) {
    LOG_TRACE << _tag << "Start requestTasks";

    try {
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
            return false;
        }

        TESTER_INFO info = {0x19, 0, (uint8_t)Dispatcher::instance().getName().length()};

        auto dataLen = sizeof(ST_SIZES) + sizeof(ST_T_HDR) + sizeof(TESTER_INFO) +
                Dispatcher::instance().getName().length();

        const uint32_t size = sizeof(ST_T_HDR) + sizeof(TESTER_INFO) +
                Dispatcher::instance().getName().length();

        ST_SIZES sizes = {0, size, 0};
        ST_T_HDR hdr = {_tid, MODULE_READ_OBJECTS_CFG};

        std::vector<uint8_t> buffer(dataLen);

        memcpy(buffer.data(), &sizes, sizeof(ST_SIZES));
        memcpy(buffer.data() + sizeof(ST_SIZES), &hdr, sizeof(ST_T_HDR));
        memcpy(buffer.data() + sizeof(ST_SIZES) + sizeof(ST_T_HDR), &info, sizeof(TESTER_INFO));
        memcpy(buffer.data() + sizeof(ST_SIZES) + sizeof(ST_T_HDR) + sizeof(TESTER_INFO),
               Dispatcher::instance().getName().c_str(), Dispatcher::instance().getName().length());

        uint32_t table[256];
        crc32::generate_table(table);

        uint32_t crc = crc32::update(table, 0xFFFFFFFF, buffer.data(), dataLen);

        memcpy(buffer.data() + sizeof(uint32_t)*2, &crc, sizeof(uint32_t));

        LOG_TRACE << _tag << "Request tasks.. ";
        dash::hex_dump(std::string(reinterpret_cast<char*>(buffer.data()), dataLen),
                       LOG_TRACE << _tag);

        boost::asio::write(socket, boost::asio::buffer(buffer, dataLen));

        auto messageSize = 64;
        std::vector<uint8_t> dataBuffer;

        boost::system::error_code ec;

        while (ec != boost::asio::error::eof) {

            std::vector<char> socketBuffer(messageSize);

            size_t received = socket.read_some(
                        boost::asio::buffer(socketBuffer), ec);

            if (received > 0) {
                LOG_TRACE << _tag << "Received from receiver " << std::dec << received;
                dash::hex_dump(std::string(socketBuffer.data(), socketBuffer.size()),
                               LOG_TRACE << _tag);
                dataBuffer.insert(dataBuffer.end(),
                                  socketBuffer.begin(), socketBuffer.begin() + received);
            }
        }

        if(ec && !boost::asio::error::eof){
            LOG_ERROR << _tag <<"Tasks request has error "<< ec.message();
        }


        if(dataBuffer.size() != 0) {
            LOG_TRACE << _tag << "Request received buffer size: " << std::dec << dataBuffer.size();
            dash::hex_dump(std::string(reinterpret_cast<char*>(dataBuffer.data()), dataBuffer.size()),
                           LOG_TRACE << _tag);

            if(dataBuffer.size() <= 8) {
                int32_t error;
                memcpy(&error, dataBuffer.data(), sizeof(int32_t));
                LOG_INFO << _tag << "Requestor received message: " << rc_error(error);
            }

            if(dataBuffer.size() > 8) {
                uint8_t* p = dataBuffer.data();

                ST_SIZES sizes = (*(ST_SIZES*)p);
                sizes.print(LOG_TRACE << _tag);

                p += sizeof(ST_SIZES);

                std::vector<uint8_t> decompressed;
                if(sizes.CmprSize != 0) {
                    // Decompress buffer
                    decompressed.resize(sizes.UncmprSize);
                    uLongf decompressed_len;
                    auto ret = uncompress(decompressed.data(), &decompressed_len, p, sizes.CmprSize);
                    if(ret == Z_OK) {
                        p = decompressed.data();
                    } else {
                        throw std::runtime_error("requestTasks: error uncompress tasks..");
                    }
                }


                (*(ST_T_HDR*)p).print(LOG_TRACE << _tag);

                auto count = (*(ST_T_HDR*)p).TesterId;
                p += sizeof(ST_T_HDR);

                TESTER_CFG_ADDDATA extConfig = *((TESTER_CFG_ADDDATA*)p);
                extConfig.print(LOG_TRACE << _tag);

                Dispatcher::instance().setExtConfig(extConfig);

                p += sizeof(TESTER_CFG_ADDDATA);

                sTesterConfigRecord* record = (sTesterConfigRecord*)p;

                for(auto i = 0; i < count; ++i) {
                    if(record->LObjId == 0) {
                        continue;
                    }
                    TesterConfigRecord classRecord(record);
                    Dispatcher::instance().insertRecord(record);
                    record = record->GetNext();
                }
            }
        }
    }
    catch(std::runtime_error e) {
        LOG_ERROR << _tag << "Requestor get tasks failed: exception " << e.what() << "..";
        return false;
    }

    LOG_INFO << _tag << "Number of all tasks: " << Dispatcher::size();
    Dispatcher::instance().print(LOG_TRACE << _tag);

    return true;
}

/**
 * @brief tickTimer - Have tick from global timer for synchronizing internal module loop
 */
void Requestor::tickTimer() {
    ++_tickCounter;
}

/**
 * @brief isReady - Return readynest of to be start loop iteration
 * @param level
 * @return
 */
bool Requestor::isReady(uint32_t level) {
    if(_tickCounter == level) {
        _tickCounter = 0;
        return true;
    }
    return false;

}


}
