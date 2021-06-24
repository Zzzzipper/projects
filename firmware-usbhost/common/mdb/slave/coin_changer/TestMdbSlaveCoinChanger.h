#ifndef COMMON_MDB_SLAVE_TESTCOINCHANER_H_
#define COMMON_MDB_SLAVE_TESTCOINCHANER_H_

#include "MdbSlaveCoinChanger.h"

class TestMdbSlaveCoinChanger : public MdbSlaveCoinChangerInterface {
public:
	TestMdbSlaveCoinChanger(StringBuilder *str) : str(str), reseted(false), enabled(false) {}
	void setReseted(bool reseted) { this->reseted = reseted; }
	void setEnabled(bool enabled) { this->enabled = enabled; }

	virtual void reset() { *str << "<SCC::reset>"; reseted = true; }
	virtual bool isReseted() { return reseted; }
	virtual bool isEnable() { return enabled; }
	virtual void deposite(uint8_t, uint8_t) { *str << "<SCC::deposite>"; }
	virtual void dispenseComplete() { *str << "<SCC::dispenseComplete>"; }
	virtual void escrowRequest()  { *str << "<SCC::escrowRequest>"; }

private:
	StringBuilder *str;
	bool reseted;
	bool enabled;
};

#endif
