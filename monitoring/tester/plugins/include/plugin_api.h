#pragma once

#include "iplugin.h"
#include "log.h"
#include "protocol.h"
#include "json.h"

/**
 * @brief The Executor class
 */

class Executor  {
public:
    Executor(uint32_t id);
    virtual ~Executor();

    virtual int32_t setExtendData(char* data, int length);

    virtual int32_t getReport(void** data);

    virtual int32_t addRefToWorker(IWorker *worker);

    virtual int32_t execute(uint32_t delay);

    void stop();

    int32_t executeCommand(const std::string&  command, std::string&        output,
                           const std::string&  mode = "r");
protected:

    virtual int32_t run() = 0;

    sTesterConfigRecord _record;
    json::Value _extData;
    std::string _hostName;
    int32_t _result;

private:

    static uint32_t count;

    uint32_t _id;

    IWorker* _worker;

    std::thread _executorThread;
};

/**
 * @brief The PluginApiClass class
 */

template <class T>
class PluginApiClass : public IPluginApi {
public:

    PluginApiClass() = default;
    virtual ~PluginApiClass() {
        executors.clear();
    }

    virtual int32_t SendExtendData(uint32_t id, char* data, int length) override final {
        LOG_TRACE << "[PAPI] " << "Start SendExtendData (" << id << ")";
        const std::lock_guard lock(mutex);
        if(executors.find(id) == executors.end()) {
            return -1;
        }
        return executors.at(id)->setExtendData(data, length);
    }

    virtual int32_t GetReport(uint32_t id, void** data) override final {
        LOG_TRACE << "[PAPI] " << "Start GetReport (" << id << ")";
        const std::lock_guard lock(mutex);
        if(executors.find(id) == executors.end()) {
            LOG_TRACE << "[PAPI] " << "OnGetReport not found (" << id << ")";
            return -1;
        }
        return executors.at(id)->getReport(data);
    }

    virtual int32_t RegisterWorker(uint32_t id, IWorker* worker) override final {
        LOG_TRACE << "[PAPI] " << "Start RegisterWorker (" << id << ")";
        const std::lock_guard lock(mutex);
        if(executors.find(id) == executors.end()) {
            executors[id] = std::unique_ptr<T>(new T(id));
        }
        return executors.at(id)->addRefToWorker(worker);
    }

    virtual int32_t Execute(uint32_t id, uint32_t delay) override final {
        LOG_TRACE << "[PAPI] " << "Start Execute..";
        const std::lock_guard lock(mutex);
        if(executors.find(id) == executors.end()) {
            return -1;
        }
        return executors.at(id)->execute(delay);
    }

    virtual int32_t Stop(uint32_t id) override final {
        LOG_TRACE << "[PAPI] " << "Start Stop..";
        const std::lock_guard lock(mutex);
        if(executors.find(id) == executors.end()) {
            return -1;
        }
        executors.at(id)->stop();
        executors.erase(id);
        return 0;
    }

    virtual int32_t UnRegisterWorker(uint32_t id) override final {
        LOG_TRACE << "[PAPI] " << "Start UnRegisterWorker (" << id << ")";
        const std::lock_guard lock(mutex);
        if(executors.find(id) == executors.end()) {
            return -1;
        }
        executors.at(id)->addRefToWorker(nullptr);
        return 0;
    }

    virtual int32_t BindFileLogger() override final {
        return 0;
    }

private:

    std::map<uint32_t, std::unique_ptr<T>> executors;

    std::mutex mutex;

};
