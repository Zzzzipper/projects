#ifndef UTILS_TESTLED_H
#define UTILS_TESTLED_H

#include "utils/include/LedInterface.h"
#include "utils/include/StringBuilder.h"

class TestLed : public LedInterface {
public:
	TestLed(StringBuilder *result) : result(result) {}
	virtual void setInternet(State state) { *result << "<ledInternet=" << state << ">"; }
	virtual void setPayment(State state) { *result << "<ledPayment=" << state << ">"; }
	virtual void setFiscal(State state) { *result << "<ledFiscal=" << state << ">"; }
	virtual void setServer(State state) { *result << "<ledServer=" << state << ">"; }

private:
	StringBuilder *result;
};

#endif
