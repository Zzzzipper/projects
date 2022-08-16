#pragma once

#include "ievent.h"

class IObserver {
public:
    /// Accept event from event engine
    virtual void receive(IEvent& e) = 0;

    /// Have tick from global timer for synchronizing internal module loop
    virtual void tickTimer() = 0;

    /// Return readynest of to be start loop iteration
    virtual bool isReady(uint32_t level) = 0;
};
