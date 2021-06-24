#ifndef COMMON_MDB_MASTER_TESTBILLVALIDATOR_H_
#define COMMON_MDB_MASTER_TESTBILLVALIDATOR_H_

#include "MdbMasterBillValidator.h"

class TestMdbMasterBillValidator : public MdbMasterBillValidatorInterface {
public:
	TestMdbMasterBillValidator(uint16_t deviceId, StringBuilder *str) : deviceId(deviceId), str(str), inited(false) {}
	void setInited(bool inited) { this->inited = inited; }

	virtual EventDeviceId getDeviceId() { return deviceId; }
	virtual void reset() { *str << "<MBV::reset()>"; }
	virtual bool isInited() { return inited; }
	virtual void disable() { *str << "<MBV::disable>"; }
	virtual void enable() { *str << "<MBV::enable>"; }

private:
	EventDeviceId deviceId;
	StringBuilder *str;
	bool inited;
};

#endif
