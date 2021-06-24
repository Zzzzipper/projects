#ifndef COMMON_MDB_MASTER_TESTCOINCHANER_H_
#define COMMON_MDB_MASTER_TESTCOINCHANER_H_

#include "MdbMasterCoinChanger.h"

class TestMdbMasterCoinChanger : public MdbMasterCoinChangerInterface {
public:
	TestMdbMasterCoinChanger(uint16_t deviceId, StringBuilder *str) : deviceId(deviceId), str(str), inited(false), change(false) {}
	void setInited(bool inited) { this->inited = inited; }
	void setChange(bool change) { this->change = change; }

	virtual EventDeviceId getDeviceId() { return deviceId; }
	virtual void reset() { *str << "<MCC::reset>"; }
	virtual bool isInited() { return inited; }
	virtual bool hasChange() { return change; }
	virtual void disable() { *str << "<MCC::disable>"; }
	virtual void enable() { *str << "<MCC::enable>"; }
	virtual void dispense(const uint32_t sum) { *str << "<MCC::dispense(" << sum << ")>"; }
	virtual void dispenseCoin(uint8_t data) { *str << "<MCC::dispenseCoin(" << data << ")>"; }
	virtual void startTubeFilling() { *str << "<MCC::startTubeFilling>"; }
	virtual void stopTubeFilling() { *str << "<MCC::stopTubeFilling>"; }

private:
	EventDeviceId deviceId;
	StringBuilder *str;
	bool inited;
	bool change;
};

#endif
