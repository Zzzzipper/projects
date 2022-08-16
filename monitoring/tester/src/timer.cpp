#include "timer.h"


namespace tester {
Timer::Timer() {}

/**
 * @brief Timer::setObserver- Insert IObserver object in to internal list it
 * @param o
 */
void Timer::setObserver(IObserver* o) {
    _observers.push_back(o);
}


/**
 * @brief Timer::tick - Send tick pulse to oservers
 */
void Timer::tick() {
    for(auto o: _observers) {
        o->tickTimer();
    }
}

}
