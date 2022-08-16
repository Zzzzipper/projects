#pragma once

#include "iobserver.h"
#include "dispatcher.h"

namespace tester {

/**
 * @brief The Sender class - entity for sending report to receiver
 */
class Sender : public IObserver  {
public:
    explicit Sender();
    explicit Sender(uint32_t tid);

    /// Upload results
    void uploadResults(const char* host, const char* port);

    /// Accept event from event engine
    virtual void receive(IEvent& e) final {}

    /// Have tick from global timer for synchronizing internal module loop
    virtual void tickTimer() final;

    /// Return readynest of to be start loop iteration
    virtual bool isReady(uint32_t level = 0) final;

private:
    /// Tester index
    const uint32_t _tid;
    uint32_t _tickCounter;
};

}
