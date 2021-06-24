#ifndef _headmeters_h_
#define _headmeters_h_

#include <vector>
#include <string>
#include "message.h"

class Head {

public:
    explicit Head(std::string name_);
    std::string &state();   // Channel state
    std::string &name();    // Channel name

private:
    void _setBusy();
    void _setIdled();
    void _setMeasuring();
    void _setError();
    std::string _name;
    std::string _state;
    std::string _error;
};


class HeadMeters {

public:
    static HeadMeters* instance();
    static void* operate(void* request_);
    static void* getStatus(void* request_);
    explicit HeadMeters();
    virtual ~HeadMeters();
    Head* operator[](int num_);

private:
    static HeadMeters* _instance;

private:
    void* _pack(std::string answer_, uint64_t index_, uds_command_type commit_);
    std::vector<Head*> _heads = {NCHANNELS, nullptr};

};


#endif //_headmeters_h_
