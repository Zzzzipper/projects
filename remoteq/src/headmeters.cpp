#include <memory>
#include <functional>
#include "headmeters.h"
#include "loguru.h"

HeadMeters* HeadMeters::_instance = nullptr;

// The prefix channel string identificator
const static char* prefix = "channel";
/**
 * @brief Head::Head
 * @param name_
 */
Head::Head(std::string name_)
    : _name(name_)
{
    _setIdled();
}

/**
 * @brief Head::name
 * @return
 */
std::string &Head::name() {
    return _name;
}

/**
 * @brief Head::state
 * @return
 */
std::string &Head::state() {
    return _state;
}

/**
 * @brief Head::_setBusy
 */
void Head::_setBusy() {
    _state = "busy_state";
}

/**
 * @brief Head::_setIdled
 */
void Head::_setIdled() {
    _state = "idle_state";
}

/**
 * @brief Head::_setMeasuring
 */
void Head::_setMeasuring() {
    _state = "measure_state";
}

/**
 * @brief Head::_setError
 */
void Head::_setError() {
    _state = "error_state";
}

/**
 * @brief HeadMeters::instance
 * @return
 */
HeadMeters* HeadMeters::instance() {
    if(!_instance) {
        _instance = new HeadMeters();
    }
    return _instance;
}

/**
 * @brief HeadMeters::HeadMeters
 */
HeadMeters::HeadMeters() {
    int num = 0;
    for(std::vector<Head*>::iterator iit = _heads.begin(); iit != _heads.end(); ++iit, ++num) {
        (*iit) = new Head(std::string(prefix) + std::to_string(num));
    }
}

/**
 * @brief HeadMeters::operator []
 * @param num_
 * @return
 */
Head* HeadMeters::operator[](int num_) {
    return _heads.at(static_cast<size_t>(num_));
}

/**
 * @brief HeadMeters::~HeadMeters
 */
HeadMeters::~HeadMeters() {
    for(std::vector<Head*>::iterator iit = _heads.begin(); iit != _heads.end(); ++iit) {
        delete (*iit);
    }
    LOG_F(INFO, "Deleted %d measurement heads..", NCHANNELS);
}

/**
 * @brief HeadMeters::operate - Universal dispather for operate headmeters.
 *                              Worked on reassigned lambda parsers and encoders.
 * @param request_
 * @return
 */
void* HeadMeters::operate(void *request_) {
    UdsMessage message(request_);
    auto f = +[](UdsMessage&){ return std::string("fail"); };
    switch(message.command()) {
    //-----------------------------------------------------------------------------
    case CMD_GET_STATUS:
        f = +[](UdsMessage& message) {
            // Get numeric number of channel
            int channel = std::atoi(message.data().c_str());
            std::string answer = "ok,"
                    + std::to_string(channel)/*so that not drop 0*/
                    + "," + (*instance())[channel]->state();
            return answer;
        };
        break;
    //-----------------------------------------------------------------------------
    case CMD_CHANNELS_INFO:
        f = +[](UdsMessage&) {
            HeadMeters* hm = instance();
            std::string answer = "ok,";
            std::vector<Head*>::iterator iit = hm->_heads.begin();
            for(iit = hm->_heads.begin(); iit != hm->_heads.end(); ++iit) {
                answer += (*iit)->name() + ",";
            }
            answer.pop_back(); /* remove last , */
            LOG_F(INFO, "Answer is %s", answer.c_str());
            return answer;
        };
        break;

    default:
        break;

    }
    return instance()->_pack(f(message), message.index(), message.command());
}

/**
 * @brief HeadMeters::_pack - Pack answer to responce buffer
 * @param answer_
 * @return
 */
void* HeadMeters::_pack(std::string answer_, uint64_t index_, uds_command_type commit_) {
    ssize_t len = (answer_.empty())?
                0: static_cast<ssize_t>(sizeof(uds_command_t) + answer_.length() + 1);
    // Free memoory block make in c-code !
    void* result = malloc(static_cast<size_t>(len));
    (reinterpret_cast<uds_command_t*>(result))->status = STATUS_SUCCESS;
    (reinterpret_cast<uds_command_t*>(result))->commit = commit_;
    (reinterpret_cast<uds_command_t*>(result))->index = index_;
    (reinterpret_cast<uds_command_t*>(result))->data_len = static_cast<uint32_t>(len);
    if(len != 0) {
        memcpy((char*)result + sizeof(uds_command_t), answer_.c_str(), answer_.length());
        ((char*)result)[len - 1] = '\0';
    }
    return result;
}

