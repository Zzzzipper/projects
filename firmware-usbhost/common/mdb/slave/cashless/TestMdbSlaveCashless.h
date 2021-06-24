#ifndef COMMON_MDB_SLAVE_TESTCASHLESS_H_
#define COMMON_MDB_SLAVE_TESTCASHLESS_H_

#include "MdbSlaveCashless3.h"

class TestMdbSlaveCashless : public MdbSlaveCashlessInterface {
public:
	TestMdbSlaveCashless(uint16_t deviceId, StringBuilder *str) : deviceId(deviceId), str(str), inited(false), enabled(false) {}
	void setInited(bool inited) { this->inited = inited; }
	void setEnabled(bool enabled) { this->enabled = enabled; }

	virtual EventDeviceId getDeviceId() { return deviceId; }
	virtual void reset() { *str << "<SCL::reset>"; }
	virtual bool isInited() { return inited; }
	virtual bool isEnable() { return enabled; }
	virtual void setCredit(uint32_t credit) { *str << "<SCL::setCredit(" << credit << ")>"; }
	virtual void approveVend(uint32_t productPrice) { *str << "<SCL::approveVend=" << productPrice << ">"; }
	virtual void denyVend(bool ) { *str << "<SCL::denyVend>"; }
	virtual void cancelVend() { *str << "<SCL::cancelVend>"; }

private:
	EventDeviceId deviceId;
	StringBuilder *str;
	bool inited;
	bool enabled;
};

#endif
