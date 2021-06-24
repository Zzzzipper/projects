#ifndef COMMON_MDB_SLAVE_TESTBILLVALIDATOR_H_
#define COMMON_MDB_SLAVE_TESTBILLVALIDATOR_H_

#include "MdbSlaveBillValidator.h"

class TestMdbSlaveBillValidator : public MdbSlaveBillValidatorInterface {
public:
	TestMdbSlaveBillValidator(StringBuilder *str) : str(str), reseted(false), enabled(false) {}
	void setReseted(bool reseted) { this->reseted = reseted; }
	void setEnabled(bool enabled) { this->enabled = enabled; }

	void reset() override { *str << "<SBV::reset>"; reseted = true; }
	bool isReseted() override { return reseted; }
	bool isEnable() override { return enabled; }
	bool deposite(uint8_t index) { *str << "<SBV::deposite(" << index << ">"; return true; }

private:
	StringBuilder *str;
	bool reseted;
	bool enabled;
};

#endif
