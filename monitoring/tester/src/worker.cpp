#include <sstream>
#include <iostream>

#include "worker.h"
#include "processor.h"
#include "json.h"
#include "m_types.h"
#include "worker.h"
#include "scope.h"
#include "utils.h"

static const char* _tag = "[WORKER] ";
static const uint32_t _reReadPeriod = 60;

namespace tester {

Worker::Worker(WorkerManager& manager, TesterConfigRecord& record)
    :
      _worker_manager(manager),
      _taskLibrary(nullptr),
      _record(record),
      _responseTick(0),
      _isTimerRunnig(false),
      _isWorkerStarted(false),
      _job_mutex(),
      _ready(),
      _breakDelay(BREAK_CONN_INTERVAL * 1000),
      _taken(0)
{
    static uint32_t counter;
    if(counter > 1e6) {
        counter = 0;
    }
    counter++;

    _id = counter;
}

bool Worker::pluginIsReady() {
    try {
        _taskLibrary = tester::Holder::instance().getLibraryByTask(_record.ModType);

        if(_taskLibrary != nullptr) {
            _initPluginName();

            if(_registerWorker() == 0) {
                LOG_TRACE << _tag << "Worker " << _id
                          << " " << _pluginName << " registered succesfully..";
                _sendExtendData();
            }

            return true;

        } else {
            LOG_ERROR << _tag << "Error loading plugin library " << _record.ModType;
        }
    }
    catch(std::exception &e) {
        LOG_ERROR << _tag << e.what();

    }
    return false;
}

Worker::~Worker() {
    if(_taskLibrary) {
        if(_workerThread.joinable()) {
            _workerThread.join();
        }

        sChgRecord* record = new sChgRecord;
        DEFER({ delete record; });

        // If plugin came out without response, then
        // assign error to report record by overtime
        if(_getReport((void**)&record) == -1) {
            memset(record, 0, sizeof(sChgRecord));
            record->LObjId = _record.LObjId;
            record->CheckRes = 1;
            record->CheckStatus = 0;
            record->DelayMS = _breakDelay;
            record->baseIndex = _record.baseIndex;
        } else {
            record->DelayMS = _taken/1000;
        }

        record->ModType = _record.ModType;

        Dispatcher::instance().addResult(*record);

        LOG_INFO << _tag
                 << "base: " << _record.baseIndex
                 << ", type: " << taskNameById(_record.ModType)
                 << ", ip: "  << ipNumericToString(record->IP)
                 << ", obj: " << record->LObjId
                 << ", res: " << ((record->CheckRes == 0)? "Success": "Failed");

        _unRegisterWorker();
        _stopPlugin();

        static uint32_t alltime;
        static double everage;
        alltime += _taken;
        everage = 1.0 * alltime/_id;
        LOG_TRACE << _tag << "Et: " << everage/1000;
        LOG_TRACE << _tag << "Destruct worker success..";
    }
}

int32_t Worker::GetTask(void** record) {
    memcpy(*record, &_record, sizeof(sTesterConfigRecord));
    return 0;
}

/**
 * @brief worker::start
 */
void Worker::start() {

    if(_taskLibrary == nullptr) {
        _worker_manager.stop(shared_from_this());
        return;
    }

    LOG_TRACE << _tag << "Start worker " << _pluginName
              << "(" << _id << ")";

    _responseTimer = std::thread([&](){
        while(_responseTick < _breakDelay) {
            _isTimerRunnig = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(TICK_INTERVAL_MSEC));
            LOG_TRACE << _tag << "...";
            _responseTick += TICK_INTERVAL_MSEC;
            if(_workIsDone) {
                _isTimerRunnig = false;
                break;
            }
        }
        _worker_manager.stop(shared_from_this());
    });
    _responseTimer.detach();

    _runExecute();

}

void Worker::stop()
{
    LOG_TRACE << _tag << "Stop worker " << _pluginName
              << "(" << _id << ")";
}


void Worker::Break(uint32_t taken) {
    LOG_TRACE << _tag << "Break from plugin..";

    std::unique_lock<std::mutex> lock(_job_mutex);
    _ready.notify_one();

    _taken = taken;

}


bool Worker::_waitResult(std::unique_lock<std::mutex> &locker) {

    _ready.wait_for(locker, std::chrono::milliseconds(_breakDelay));

    LOG_TRACE << _tag << "...unlocked";

    if(_responseTick < _breakDelay) {
        LOG_TRACE << _tag << "...work is done";
        _workIsDone = true;
    }

    return _workIsDone;
}

void Worker::_runExecute() {
    try {
        LOG_TRACE << _tag << "...fire worker job";
        _workerThread = std::thread([&](){

            _isWorkerStarted = true;

            if(_execute() != 0) {
                LOG_ERROR << _tag << "Worker " << _pluginName
                          << "(" << _id << ") can't run execute..";
                return;
            }

            std::unique_lock<std::mutex> lock(_job_mutex);
            _waitResult(lock);

            return;
        });
    } catch(std::exception &e) {
        LOG_ERROR << _tag << "runExecute: " << e.what();
    }
}

int32_t Worker::_registerWorker() {
    if(_taskLibrary) {
        ldd::Function<int (uint32_t, IWorker*)> registerWorker(_taskLibrary, "RegisterWorker");
        return registerWorker(_id, this);
    }
    return -1;
}

int32_t Worker::_unRegisterWorker() {
    if(_taskLibrary) {
        ldd::Function<int (uint32_t)> unRegisterWorker(_taskLibrary, "UnRegisterWorker");
        return unRegisterWorker(_id);
    }
    return -1;
}

int32_t Worker::_stopPlugin() {
    if(_taskLibrary) {
        ldd::Function<int (uint32_t)> stopPlugin(_taskLibrary, "Stop");
        return stopPlugin(_id);
    }
    return -1;
}

int32_t Worker::_execute() {
    if(_taskLibrary) {
        ldd::Function<int (uint32_t, uint32_t)> execute(_taskLibrary, "Execute");
        return execute(_id, _breakDelay);
    }
    return -1;
}

int32_t Worker::_getReport(void** data) {
    if(_taskLibrary) {
        ldd::Function<int (uint32_t, void**)> getReport(_taskLibrary, "GetReport");
        return getReport(_id, data);
    }
    return -1;

}

int32_t Worker::_sendExtendData() {
    if(_taskLibrary) {

        json::Object data = parseNetStringToJsonObject(_record.config());
        LOG_TRACE << _tag << "Netstring: " << json::Serialize(data);

        std::string address = extractDomainName(_record.hostName());
        if(address.length() == 0) {
            address = ipNumericToString(_record.IP);
        }
        data["hostname"] = address;

        auto stream = json::Serialize(data);

        ldd::Function<int (uint32_t, char*, int)> sendExtendData(_taskLibrary, "SendExtendData");
        return sendExtendData((unsigned int)_id, stream.data(), stream.length());
    }
    return -1;
}

void Worker::_initPluginName() {
    if(_taskLibrary) {
        ldd::Function<const char* ()> name(_taskLibrary, "GetPluginName");
        _pluginName = name();
    }
}


WorkerManager::WorkerManager()
{
}

void WorkerManager::start(worker_ptr c)
{
    std::unique_lock<std::mutex> lock(_control_mutex);
    _sessions.insert(c);
    c->start();
}

void WorkerManager::stop(worker_ptr c)
{
    std::unique_lock<std::mutex> lock(_control_mutex);
    _sessions.erase(c);
    c->stop();
}

void WorkerManager::stop_all()
{
    std::unique_lock<std::mutex> lock(_control_mutex);
    for (auto c: _sessions)
        c->stop();
    _sessions.clear();
}



/**
 * @brief server::server
 * @param io_context
 * @param port
 */
Launcher::Launcher()
    : _worker_manager()
{
}


/**
 * @brief server::doAccept
 */
void Launcher::doAccept(TesterConfigRecord &record) {
    auto worker = std::make_shared<Worker>(_worker_manager, record);
    if(worker->pluginIsReady()) {
        _worker_manager.start(worker);
    } else {
        worker.reset();
    }
}

} // namespace tester



