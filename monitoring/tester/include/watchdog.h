#pragma once

#include <boost/signals2/signal.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

namespace tester {

/**
 * @brief The Runnable class
 */
class Runnable
{
    typedef boost::signals2::signal<void ()> timeout_slot;

public:
    typedef timeout_slot::slot_type timeout_slot_t;

public:
    Runnable()
        : _interval(0)
        , _is_active(false)
    {}

    Runnable(int interval)
        : _interval(interval)
        , _is_active(false)
    {}

    virtual ~Runnable() {
        stop();
    }

    inline boost::signals2::connection connect(const timeout_slot_t& subscriber) {
        return _signalTimeout.connect(subscriber);
    };

    void start() {
        boost::lock_guard<boost::mutex> lock(_guard);

        if (is_active())
            return; // Already executed.

        if (_interval <= 0)
            return;

        _runnable_thread.interrupt();
        _runnable_thread.join();

        Worker job;
        _runnable_thread = boost::thread(job, this);

        _is_active = true;
    };

    void stop() {
        boost::lock_guard<boost::mutex> lock(_guard);

        if (!is_active())
            return; // Already executed.

        _runnable_thread.interrupt();
        _runnable_thread.join();

        _is_active = false;
    };

    inline bool is_active() const {
        return _is_active;
    };

    inline int get_interval() const {
        return _interval;
    };

    void set_interval(const int msec) {
        if (msec <= 0 || _interval == msec)
            return;

        boost::lock_guard<boost::mutex> lock(_guard);
        // Keep timer activity status.
        bool was_active = is_active();

        if (was_active)
            stop();
        // Initialize timer with new interval.
        _interval = msec;

        if (was_active)
            start();
    };

public:

    friend struct Worker;

    // The Runnable worker thread.
    struct Worker {

        void operator()(Runnable* t) {

            boost::posix_time::milliseconds duration(t->get_interval());

            try {
                while (1) {
                    boost::this_thread::sleep<boost::posix_time::milliseconds>(duration);
                    {
                        boost::this_thread::disable_interruption di;
                        {
                            t->_signalTimeout();
                        }
                    }
                }
            } catch (boost::thread_interrupted const&) {
                // Handle the thread interruption exception.
                // This exception raises on boots::this_thread::interrupt.
            }
        };
    };

protected:
    int             _interval;
    bool            _is_active;

    boost::mutex    _guard;
    boost::thread   _runnable_thread;

    // Signal slots
    timeout_slot    _signalTimeout;
};
}

