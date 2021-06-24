#ifndef _message_h_
#define _message_h_

#include <vector>
#include <condition_variable>
#include <string>
#include "common.h"

/**
 * @brief The UdsMessage class - universal container
 * for data exchange
 */
class UdsMessage {

public:
    explicit UdsMessage(void* message_);
    explicit UdsMessage(uds_command_type command_, const char* message_=nullptr);
    virtual ~UdsMessage() {}
    // The full buffer of message
    char* body();
    // The messge only
    std::string data();
    // Full buffer length
    int64_t length();
    // Get commant value
    uds_command_type command();
    // Set index for send control
    void setIndex(uint64_t index_);
    // Get the responce index
    uint64_t index();

private:
    uint16_t _computeChecksum(void *buf, ssize_t len);
    std::vector<char> _data;
    std::mutex        _mutex;
    uds_command_t* _received = nullptr;
};

#endif // _message_h_
