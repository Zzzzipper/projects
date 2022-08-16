#pragma once

#include <thread>
#include <mutex>
#include <set>
#include <atomic>
#include <memory>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace receiver {

class TesterContainer;

class Tester
        : public std::enable_shared_from_this<Tester>
{

public:
    Tester(uint32_t id, uint32_t liveTime, std::string address);
    virtual ~Tester();

    friend class TesterContainer;

    void start();
    void clear();

protected:
    TesterContainer& _container;

    std::thread _activityTimer;
    std::atomic<int> _activityTick;

    std::string _testerIpAddr;
    uint32_t _testerId;
    uint32_t _liveTime;
};

typedef std::shared_ptr<Tester> tester_ptr;

class TesterContainer {
private:
    TesterContainer();

public:
    TesterContainer(const TesterContainer&) = delete;
    TesterContainer& operator=(const TesterContainer &) = delete;
    TesterContainer(TesterContainer &&) = delete;
    TesterContainer& operator=(TesterContainer &&) = delete;

    static auto& instance(){
        static TesterContainer container;
        return container;
    }

    void accept(uint32_t, uint32_t, std::string);
    void destroy(tester_ptr tester);

private:

    std::mutex _acceptMutex;
    std::set<tester_ptr> _testers;

};

} // namespace receiver
