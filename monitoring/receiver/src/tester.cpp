#include "tester.h"
#include "log.h"
#include "config.h"
#include "dblink.h"
#include "utils.h"

static const char* _tag = "[TESTER] ";
using namespace receiver;

Tester::Tester(uint32_t id, uint32_t liveTime, std::string address)
    : _container(TesterContainer::instance()),
      _activityTick(0),
      _testerId(id),
      _testerIpAddr(address),
      _liveTime(liveTime)
{
}

void Tester::start() {
    LOG_DEBUG << _tag
              << "Tester accepted: id " << _testerId
              << ", liveTime " << _liveTime
              << ", addrress " << _testerIpAddr;

    _activityTimer = std::thread([&](){
        while(_activityTick < _liveTime) {
            std::this_thread::sleep_for(std::chrono::seconds(TICK_INTERVAL_SEC));
            LOG_TRACE << _tag << "..fire tick tester live timer " << _activityTick;
            _activityTick++;
        }
        _container.destroy(shared_from_this());
    });
}

void Tester::clear() {
    _activityTimer.detach();
    LOG_TRACE << _tag << "Tester thread detached";
}

Tester::~Tester() {
    LOG_DEBUG << _tag << "Tester destroyed..";
}

TesterContainer::TesterContainer() {}

void TesterContainer::accept(uint32_t id, uint32_t liveTime, std::string address)
{
    std::lock_guard<std::mutex> guard(_acceptMutex);

    LOG_TRACE << _tag << "Start accept " << id << " tester..";

    for(auto &tester: _testers) {
        if(tester->_testerId == id) {
            tester->_activityTick = 0;
            return;
        }
    }

    auto data = _testers.insert(std::make_shared<Tester>(id, liveTime, address));
    if(data.second == true) {
        data.first->get()->start();
    }

    auto numericIpAdd = ipStringToNumeric(address);
    auto dbLinkContainer = receiver::DbLinkContainer::instance().container();
    if(dbLinkContainer.size() > 0) {
        auto iit = dbLinkContainer.begin();
        for(; iit != dbLinkContainer.end(); ++iit) {
            try {
                if(iit->second.connect()) {
                    iit->second.execFunc(std::string("call receiver_set_tester_online_state($1::integer, $2::bigint, $3::boolean);"),
                                         id, numericIpAdd, true);
                }
            }
            catch(std::exception const &e) {
                LOG_ERROR << _tag << "source: "
                          << iit->second.dbAlias() << ", tester id: " << id << " - " << e.what();
            }
        }
    }

    LOG_DEBUG << _tag << "Accepted success";
}

void TesterContainer::destroy(tester_ptr tester)
{
    std::lock_guard<std::mutex> guard(_acceptMutex);

    LOG_DEBUG << _tag << "Start destroy tester "
              << tester->_testerId << ", _activityTick " << tester->_activityTick;

    auto numericIpAdd = ipStringToNumeric(tester->_testerIpAddr);
    auto dbLinkContainer = receiver::DbLinkContainer::instance().container();
    if(dbLinkContainer.size() > 0) {
        auto iit = dbLinkContainer.begin();
        for(; iit != dbLinkContainer.end(); ++iit) {
            try {
                if(iit->second.connect()) {
                    iit->second.execFunc(std::string("call receiver_set_tester_online_state($1::integer, $2::bigint, $3::boolean);"),
                                         tester->_testerId, numericIpAdd, false);
                }
            }
            catch(std::exception const &e) {
                LOG_ERROR << _tag << "source: "
                          << iit->second.dbAlias() << ", tseter id: " << tester->_testerId << " - " << e.what();
            }
        }
    }

    _testers.erase(tester);
    tester->clear();
}

