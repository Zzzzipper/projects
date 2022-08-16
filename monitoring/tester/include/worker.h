#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <condition_variable>
#include "iplugin.h"
#include "holder.h"
#include "config.h"
#include "dispatcher.h"

namespace  tester {

class WorkerManager;

using namespace boost::interprocess;

/**
 * @brief The Worker entity
 */
class Worker :
        public std::enable_shared_from_this<Worker>,
        public IWorker
{
public:
    Worker(WorkerManager& manager, TesterConfigRecord& record);
    virtual ~Worker();

    /// Start the first asynchronous operation for the Connection.
    void start();

    /// Stop all asynchronous operations associated with the Connection.
    void stop();

    /// Check loadable plugin
    bool pluginIsReady();

    virtual int32_t GetTask(void** record) override final;
    virtual int32_t Execute() override final { return 0; }
    virtual int32_t Status() override final { return 0; }
    virtual void Break(uint32_t) override final;

private:
    void _initPluginName();

    int32_t _registerWorker();
    int32_t _unRegisterWorker();

    int32_t _stopPlugin();
    int32_t _execute();
    int32_t _getReport(void** data);

    int32_t _sendExtendData();

    /// Perform an asynchronous read operation.
    void _runExecute();

    /// Wait result
    bool _waitResult(std::unique_lock<std::mutex>& locker);

    /// The manager for this Connection.
    WorkerManager& _worker_manager;

    ldd::Library* _taskLibrary;

    TesterConfigRecord& _record;

    std::thread _responseTimer;

    std::atomic<int> _responseTick;

    std::atomic<bool> _workIsDone;
    std::atomic<bool> _isTimerRunnig;
    std::atomic<bool> _isWorkerStarted;

    std::thread _workerThread;

    std::mutex _job_mutex;
    std::condition_variable  _ready;

    size_t _breakDelay;

    uint32_t _id;
    uint32_t _taken;

    std::string _pluginName;

};

typedef std::shared_ptr<Worker> worker_ptr;

/**
 * @brief The WorkerManager class -
 * Manages open connections so that they may be cleanly stopped when the server
 * needs to shut down.
 */
class WorkerManager
{
public:
    WorkerManager(const WorkerManager&) = delete;
    WorkerManager& operator=(const WorkerManager&) = delete;

    /// Construct a connection manager.
    WorkerManager();

    /// Add the specified connection to the manager and start it.
    void start(worker_ptr);

    /// Stop the specified connection.
    void stop(worker_ptr);

    /// Stop all connections.
    void stop_all();

private:
    /// The managed connections.
    std::set<worker_ptr> _sessions;

    std::mutex _control_mutex;
};

/**
 * @brief The Async TCP server entity
 */
class Launcher
{
private:
    Launcher();
public:
    Launcher(const Launcher&) = delete;
    Launcher& operator=(const Launcher &) = delete;
    Launcher(Launcher &&) = delete;
    Launcher & operator=(Launcher &&) = delete;

    static auto& instance(){
        static Launcher launcher;
        return launcher;
    }

    /// Perform an asynchronous check task operation.
    void doAccept(TesterConfigRecord& record);

private:

    /// The connection manager which owns all live connections.
    WorkerManager _worker_manager;
};

} // namespace  tester
