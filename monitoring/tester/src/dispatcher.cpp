#include <chrono>

#include "holder.h"
#include "dispatcher.h"
#include "log.h"
#include "worker.h"

static const char* _tag = "[DISP] ";

namespace tester {

TESTER_CFG_ADDDATA Dispatcher::_extConfig={};
// Dispatcher::MapByBinary Dispatcher::_configs={};
Dispatcher::MapByIndex Dispatcher::_tasks={};
Dispatcher::MypByIp Dispatcher::_mapByIp={};
Dispatcher::MypByTask Dispatcher::_mapByTask={};

/**
 * @brief Dispatcher::Dispatcher
 */
Dispatcher::Dispatcher() {}

/**
 * @brief Dispatcher::getExtConfig - Get extConfig structure data
 * @return
 */
TESTER_CFG_ADDDATA& Dispatcher::getExtConfig() {
    return _extConfig;
}

/**
 * @brief Dispatcher::setExtConfig - Init TESTER_CFG_ADDDATA
 * @param extConfig
 */
void Dispatcher::setExtConfig(TESTER_CFG_ADDDATA extConfig) {
    _extConfig = extConfig;
}

void Dispatcher::clearRequests() {
    const std::lock_guard lock(_requestLocker);
    _mapByIp.clear();
    _mapByTask.clear();
    _tasks.clear();
}

/**
 * @brief Dispatcher::insertRecord - Append record
 * @param record
 */
void Dispatcher::insertRecord(sTesterConfigRecord* record) {
    const std::lock_guard lock(_requestLocker);

    TesterConfigRecord r(record);
    r.print(LOG_TRACE << _tag);

    auto index = record->baseIndex * BASE_INDEX_MULTIPLIER + r.LObjId;
    LOG_TRACE << _tag << "Task accepted, id = " << index;

#if 1
    if(_tasks.find(index) == _tasks.end()) {
        _tasks[index] = r;
        _mapByTask[r.ModType].push_back(&(_tasks[index]));
        _mapByIp[r.IP][r.ModType].push_back(&(_tasks[index]));
    } else {
        LOG_TRACE << _tag << "Packet has dublicating config..";
    }
#else
    if(_configs.find(r.hash()) == _configs.end()) {
        _configs[r.hash()] = r;
        _mapByTask[r.ModType].push_back(&(_configs[r.hash()]));
        _mapByIp[r.IP][r.ModType].push_back(&(_configs[r.hash()]));
    } else {
        LOG_TRACE << _tag << "Packet has dublicating config..";
    }
#endif
}


/**
 * @brief Dispatcher::size- Size off all records list
 * @return
 */
uint32_t Dispatcher::size() {
    return _tasks.size();
}

#ifndef RECEIVER_SIDE

void Dispatcher::addResult(sChgRecord &result) {
    const std::lock_guard lock(_resultLocker);
    auto index = result.baseIndex * BASE_INDEX_MULTIPLIER + result.LObjId;

    LOG_TRACE << _tag << "Report added, id = " << index;

    _mapResults[index] = result;
}

void Dispatcher::clearResults() {
    const std::lock_guard lock(_resultLocker);
    _mapResults.clear();
}

/**
 * @brief executeTasks - Start work with tasks
 */
void Dispatcher::executeTasks() {
    if(!size()) {
        return;
    }

    clearResults();

    LOG_INFO << _tag << "Execute tasks: " << _tasks.size();

    auto iit = _tasks.begin();
    do {
//        if(iit->second.ModType != 2) {
            tester::Launcher::instance().doAccept(iit->second);
//        }
        iit++;
    } while(iit != _tasks.end());
}

/**
 * @brief outputPack - Pack output buffer for receiver
 * @param task
 * @return
 */
std::vector<uint8_t> Dispatcher::outputPack() {
    const std::lock_guard lock(_resultLocker);

    LOG_INFO << _tag << "Report records count: " << _mapResults.size();
    if(_mapResults.size() == 0) {
        return {};
    }

    std::vector<uint8_t> out;
    auto iit = _mapResults.begin();
    do {
        std::vector<uint8_t> buffer(sizeof(sChgRecord));
        std::copy((uint8_t*)&(iit->second), (uint8_t*)&(iit->second) + sizeof(sChgRecord), buffer.begin());
        out.insert(out.end(), buffer.begin(), buffer.end());
        iit++;
        //        LOG_TRACE << response.LObjId;
    } while(iit != _mapResults.end());

    return out;
}

#endif


/**
 * @brief Dispatcher::setId
 * @param id
 */
void Dispatcher::setId(uint32_t id) {
    _id = id;
}

/**
 * @brief Dispatcher::getId
 * @return
 */
uint32_t Dispatcher::getId() const {
    return _id;
}

/**
 * @brief Dispatcher::setInfo
 * @param info
 */
void Dispatcher::setInfo(std::string info) {
    _info = info;
}

/**
 * @brief Dispatcher::getInfo
 * @return
 */
std::string Dispatcher::getInfo() const {
    return _info;
}

/**
 * @brief Dispatcher::setName
 * @param name
 */
void Dispatcher::setName(std::string name) {
    _name = name;
}

/**
 * @brief Dispatcher::getName
 * @return
 */
std::string Dispatcher::getName() {
    return _name;
}

/**
 * @brief Dispatcher::setVesrion
 * @param version
 */
void Dispatcher::setVersion(std::string version) {
    _version = version;
}

/**
 * @brief Dispatcher::getVersion
 * @return
 */
std::string Dispatcher::getVersion() {
    return _version;
}

/**
 * @brief Holder::getNumberOfTask
 * @param id
 */
std::map<int, int> Dispatcher::getNumberOfTask() {
    std::map<int,int> out;
    for(auto it = _mapByTask.begin(); it != _mapByTask.end(); it++) {
        out[it->first] = it->second.size();
    }
    return out;
}

}
