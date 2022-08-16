#pragma once

#include <list>
#include "iobserver.h"


namespace tester {

/**
 * @brief The Timer class - Entity for manage observers by tick timer
 */
class Timer {
public:
    explicit Timer();

    /// Insert IObserver object in to internal list it
    void setObserver(IObserver* o);

    /// Send tick pulse to oservers
    void tick();

private:
    std::list<IObserver*> _observers;

};
}
