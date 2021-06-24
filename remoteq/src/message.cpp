#include <iterator>
#include <algorithm>

#include "uds.h"
#include "message.h"

/**
 * @brief UdsMessage::UdsMessage - writing constructor, for request
 * @param command_
 * @param message_
 */
UdsMessage::UdsMessage(uds_command_type command_, const char* message_)
{
    uds_command_t sent;
    sent.command = command_;
    sent.data_len = (message_==nullptr)?
                0 :static_cast<uint32_t>(strlen(message_) + 1);
    std::copy((const char*)&sent
              , (const char*)&sent + sizeof(uds_command_t)
              , std::back_inserter(_data));
    if(message_) {
        std::copy(message_, message_ + strlen(message_)
                  , std::back_inserter(_data));
        _data.push_back('\0');
    }
    ssize_t len = sizeof(uds_command_t) + sent.data_len;
    // Make post preparation
    uds_command* result = reinterpret_cast<uds_command_t*>(_data.data());
    result->signature = UDS_SIGNATURE;
    result->commit = command_;
    result->checksum = 0;
    result->checksum = _computeChecksum((_data.data()), len);
}

/**
 * @brief UdsMessage::setIndex - Set index for send control
 */
void UdsMessage::setIndex(uint64_t index_) {
    uds_command* result = reinterpret_cast<uds_command_t*>(_data.data());
    if(result) {
        result->index = index_;
        result->checksum = 0;
        result->checksum = _computeChecksum((_data.data())
                                            , sizeof(uds_command_t) + result->data_len);
    }
}

/**
 * @brief UdsMessage::chekIndex - Check the responce index
 * @param index_
 * @return
 */
uint64_t UdsMessage::index() {
    uds_command* result = reinterpret_cast<uds_command_t*>(_data.data());
    return result->index;
}

/**
 * @brief UdsMessage::command - Get commant value
 * @return Value
 */
uds_command_type UdsMessage::command() {
    if(_received) {
        return static_cast<uds_command_type>(_received->commit);
    }
    return CMD_UNKNOWN;
}

/**
 * @brief UdsMessage::UdsMessage - reading constructor
 * @param message_
 */
UdsMessage::UdsMessage(void* message_)
{
    _data.clear();
    _received = reinterpret_cast<uds_command_t*>(message_);
    std::copy(reinterpret_cast<char*>(_received)
              , reinterpret_cast<char*>(_received) + sizeof(uds_command_t)
              , std::back_inserter(_data));
    std::copy(reinterpret_cast<char*>(message_) + sizeof(uds_command_t)
              , reinterpret_cast<char*>(message_) + sizeof(uds_command_t) + _received->data_len
              , std::back_inserter(_data));
}

/**
 * @brief UdsMessage::data - get message
 * @return
 */
std::string UdsMessage::data() {
    std::string result;
    try {
        if(_received) {
            // Need preallocate strongly!
            result.reserve(_received->data_len - 1);
            // Fix bug: copy use heap of ordered memory, but the _data - none
            // SIGHEAP error
            std::vector<char>::iterator iit = _data.begin() + sizeof(uds_command_t);
            size_t i = 0;
            while (iit != _data.end() - 1) {
                result[i] = (*iit);
                ++iit; ++i;

            }
            //            std::copy(_data.data() + sizeof(uds_command_t)
            //                      , _data.data() + sizeof(uds_command_t) + _received->data_len - 1 // Drop last \n
            //                      , result.begin());
        }
    } catch(const char* e) {
        //..
    }
    return result;
}

/**
 * @brief UdsMessage::body
 * @return
 */
char* UdsMessage::body() {
    return _data.data();
}

/**
 * @brief UdsMessage::length
 * @return
 */
int64_t UdsMessage::length() {
    return static_cast<int64_t>(_data.size());
}

/**
 * @brief UdsMessage::computeChecksum - Compute checksum of message
 * @param buf   - counted buffer
 * @param len   - lenth of buffer
 * @return CRC
 */
uint16_t UdsMessage::_computeChecksum(void *buf, ssize_t len) {
    uint16_t *word;
    uint8_t *byte;
    ssize_t i;
    unsigned long sum = 0;

    if (!buf) {
        return 0;
    }

    word = reinterpret_cast<uint16_t *>(buf);
    for (i = 0; i < len/2; i++) {
        sum += word[i];
    }

    /* If the length(bytes) of data buffer is an odd number, add the last byte. */
    if (len & 1) {
        byte = reinterpret_cast<uint8_t*>(buf);
        sum += byte[len-1];
    }

    /* Take only 16 bits out of the sum and add up the carries */
    while (sum>>16) {
        sum = (sum>>16) + (sum&0xFFFF);
    }

    return static_cast<uint16_t>(~sum);
}
