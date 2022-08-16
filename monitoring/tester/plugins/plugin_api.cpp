#include <cstdint>
#include <thread>

#include "plugin_api.h"
#include "scope.h"

static const char* _tag = "[EXEC] ";

uint32_t Executor::count = 0L;

Executor::Executor(uint32_t id)
    : _id(id),
      _worker(nullptr),
      _result(1)
{
    count++;
}

Executor::~Executor() {
    count--;
    LOG_TRACE << _tag << "Count: " << count;
    LOG_TRACE << _tag << "Executor " << _id
              << ": deleted pointer: " << reinterpret_cast<std::uintptr_t>(this);
}

int32_t Executor::setExtendData(char* data, int length) {
    LOG_TRACE << _tag << "Executor " << _id
              << ": set extend data pointer: " << reinterpret_cast<std::uintptr_t>(this);

    std::string buffer(data, length);
    LOG_DEBUG << _tag << "Data: " << buffer;

    // auto text = "{\"hostname\":\"yandex.ru\"}";
    _extData = json::Deserialize(buffer);

    _hostName = _extData["hostname"].ToString();

    return 0;
}

int32_t Executor::getReport(void** data) {

    const auto p1 = std::chrono::system_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::seconds>(
                p1.time_since_epoch()).count();

    sChgRecord report;

    memset(&report, 0, sizeof(sChgRecord));

    report.LObjId = _record.LObjId;
    report.PObjId = _record.PObjId;
    report.CheckDt = dt;
    report.IP = _record.IP;
    report.CheckRes = (_result == 0)? 0: 1;
    report.DelayMS = 35;
    report.CheckStatus = (_result != 0)? 0: 1;
    report.baseIndex = _record.baseIndex;

    memcpy(*data, &report, sizeof(sChgRecord));

    return 0;
}

int32_t Executor::addRefToWorker(IWorker *worker) {

    _worker = worker;

    if(_worker == nullptr) {
        return -1;
    }

    sTesterConfigRecord* record = new sTesterConfigRecord;
    DEFER({ delete record; });

    if(_worker == nullptr || _worker->GetTask((void**)&record) != 0) {
        LOG_ERROR << _tag << "Error get task object..";
        return -1;
    }

    memcpy(&_record, record, sizeof(sTesterConfigRecord));

    LOG_TRACE << _tag << "Received " << _record.LObjId << ", ModType " << _record.ModType;

    return 0;
}

int32_t Executor::execute(uint32_t delay) {
    LOG_TRACE << "Executor " << _id
              << ": execute pointer: " << reinterpret_cast<std::uintptr_t>(this);

    int32_t result = 0;

    _executorThread = std::thread([&]() {
        try {

            // This need for save object data for
            // using after the thread will be detach

            auto savedDelay = delay;

            auto start = std::chrono::high_resolution_clock::now();

            result = run();

            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

            LOG_TRACE << _tag << "Time taken by function: "
                      << duration.count()/1000.0 << " ms., savedDelay: " << savedDelay << " ms.";

            if(duration.count()/1000 < savedDelay) {
                if(_worker != nullptr) {
                    _result = result;
                    _worker->Break(duration.count());
                }
            }

        } catch (std::exception &e) {
            LOG_ERROR << _tag << "Executor: " << e.what();
        }
        return;
    });

    _executorThread.detach();

    return result;
}

void Executor::stop() {
}

/**
 * @brief Execute Generic Shell Command
 *
 * @param[in]   command Command to execute.
 * @param[out]  output  Shell output.
 * @param[in]   mode read/write access
 *
 * @return 0 for success, 1 otherwise.
 *
*/

int32_t Executor::executeCommand(const std::string& command,
                                  std::string& output, const std::string& mode)
{
    // Create the stringstream
    std::stringstream sout;

    // Run Popen
    FILE *in;
    char buff[512];

    // Test output
    if(!(in = popen(command.c_str(), mode.c_str()))){
        return 1;
    }

    // Parse output
    while(fgets(buff, sizeof(buff), in)!=NULL){
        sout << buff;
    }

    // Close
    int exit_code = pclose(in);

    // set output
    output = sout.str();

    // Return exit code
    return exit_code;
}



