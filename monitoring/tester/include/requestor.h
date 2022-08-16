#pragma once

#include <cstdint>
#include "iobserver.h"
#include "protocol.h"
#include "dispatcher.h"

namespace tester {
class Requestor : public IObserver{
public:
    explicit Requestor();
    explicit Requestor(uint32_t tid);

    /// Request and dispatch task from receiver/tester
    bool requestTasks(const char* host, const char* port);

    /// Accept event from event engine
    virtual void receive(IEvent& e) final {}

    /// Have tick from global timer for synchronizing internal module loop
    virtual void tickTimer() final;

    /// Return readynest of to be start loop iteration
    virtual bool isReady(uint32_t level = 0) final;

private:
    const uint32_t _tid;
    uint32_t _tickCounter;
};
}

