#pragma once

#include <map>
#include <list>
#include <vector>

#include "protocol.h"
#include "event.h"

#define BASE_INDEX_MULTIPLIER 10000000

namespace  tester {

class Dispatcher{
private:
    Dispatcher();

public:
    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher &) = delete;
    Dispatcher(Dispatcher &&) = delete;
    Dispatcher & operator=(Dispatcher &&) = delete;

    typedef std::map<std::string, TesterConfigRecord> MapByBinary;
    typedef std::map<uint32_t, TesterConfigRecord> MapByIndex;
    typedef std::map<uint16_t, std::list<TesterConfigRecord*>> MypByTask;
    typedef std::map<uint32_t, MypByTask> MypByIp;
    typedef std::map<uint32_t, sChgRecord> MypResults;

    static auto& instance(){
        static Dispatcher tree;
        return tree;
    }

    // Init TESTER_CFG_ADDDATA
    void setExtConfig(TESTER_CFG_ADDDATA extConfig);

    // Clear requests list
    void clearRequests();

    // Append record
    void insertRecord(sTesterConfigRecord* record);

    // Size off all records list
    static uint32_t size();

    // Set tester ID
    void setId(uint32_t id);
    uint32_t getId() const;

    void setName(std::string name);
    std::string getName();

    // Set tester info string
    void setInfo(std::string info);
    std::string getInfo() const;

    // Set/Get tester version+build
    void setVersion(std::string version);
    std::string getVersion();

    // Get number of task by them id
    static std::map<int,int> getNumberOfTask();

    // Debug print
    template<class Elem, class Traits>
    void print(std::basic_ostream<Elem, Traits>& aStream) {
#if 0
        auto i = 1;
        if(configs_.size() != 0) {
            auto iit = configs_.begin();
            do {
                std::cout << std::dec << i << ". " << "Hash=" << iit->second.hash() << std::endl;
                iit++; i++;
            } while(iit != configs_.end());
        }
        auto i = 1, j = 1;
        if(mapByTask_.size() != 0) {
            auto iit = mapByTask_.begin();
            do {
                std::cout << std::dec << i << ". " << "ModeType=" << iit->first << ":   " << std::endl;
                if(iit->second.size()) {
                    auto iip = iit->second.begin();
                    do {
                        std::cout << "\t\t" << std::dec << j << ". " << "Hash=" << (*iip)->hash() << std::endl;
                        iip++; j++;
                    } while(iip != iit->second.end());
                }
                iit++; i++;
            } while(iit != mapByTask_.end());
        }
#endif
        auto i = 1, j = 1, k = 1;
        if(_mapByIp.size() != 0) {
            auto iit = _mapByIp.begin();
            do {
                aStream << std::dec << i << ". " << "IP="
                        << (int)(uint8_t)(iit->first>>8) << "." << (int)(uint8_t)(iit->first>>4) << "."
                        << (int)(uint8_t)(iit->first>>2) << "." << (int)(uint8_t)(iit->first) << ":   ";
                if(iit->second.size()) {
                    auto iip = iit->second.begin();
                    do {
                        aStream << "\t\t" << std::dec << j << ". " << "ModeType=" << iip->first << " :   ";
                        if(iip->second.size()) {
                            auto iin = iip->second.begin();
                            do {
                                aStream << "\t\t\t\t" << std::dec << k << ". " << "Hash=" << (*iin)->LObjId;
                                iin++; k++;
                            } while(iin != iip->second.end());
                        }
                        iip++; j++;
                    } while(iip != iit->second.end());
                }
                iit++; i++;
            } while(iit != _mapByIp.end());
        }
    }

    // Get extConfig structure data
    static TESTER_CFG_ADDDATA& getExtConfig();

#ifndef RECEIVER_SIDE
    // Add result to storage
    void addResult(sChgRecord& result);
    // Clear results
    void clearResults();
    // Start work with tasks
    void executeTasks();
    // Pack output buffer for receiver
    std::vector<uint8_t> outputPack();
#endif

private:
    static TESTER_CFG_ADDDATA _extConfig;

    // All loaded configs
    // static MapByBinary _configs;
    static MapByIndex _tasks;

    // IP mapped configs
    static MypByIp _mapByIp;

    // Task mapped configs
    static MypByTask _mapByTask;

    // The map of results
    MypResults _mapResults;

    // Tester ID
    uint32_t _id;

    // Tester info
    std::string _info;

    // Tester nickname
    std::string _name;

    // Tester version and build
    std::string _version;

    // Lock data in updating procedures
    std::mutex _requestLocker;

#ifndef RECEIVER_SIDE
    // Lock data in updating procedures
    std::mutex _resultLocker;
#endif

};

}
