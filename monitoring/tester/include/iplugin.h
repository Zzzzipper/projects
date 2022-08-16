#pragma once

#include <cstdint>
#include <stddef.h>

class IWorker;

class IPluginApi {
public:
    virtual int32_t BindFileLogger() = 0;

    virtual int32_t Stop(uint32_t) = 0;

    virtual int32_t Execute(uint32_t id, uint32_t) = 0;

    virtual int32_t RegisterWorker(uint32_t, IWorker*) = 0;

    virtual int32_t UnRegisterWorker(uint32_t) = 0;

    virtual int32_t SendExtendData(uint32_t, char*, int) = 0;

    virtual int32_t GetReport(uint32_t, void**) = 0;

};

/**
 * @brief The IWorker class - worker interface
 */
class IWorker {
public:

    // Get task object for check process from worker object
    virtual int32_t GetTask(void** record) = 0;

    // Run check process
    virtual int32_t Execute() = 0;

    // Get status of checking process
    virtual int32_t Status() = 0;

    // Stop checking
    virtual void Break(uint32_t) = 0;
};

#define pluginApi(module)  pluginApi_##module
