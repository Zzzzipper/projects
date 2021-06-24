#ifndef COMMON_FISCALREGISTER_TEST_H_
#define COMMON_FISCALREGISTER_TEST_H_

#include "common/event/include/EventEngine.h"
#include "common/fiscal_register/include/FiscalRegister.h"

class FiscalTest : public EventSubscriber {
public:
	FiscalTest(EventEngineInterface *eventEngine, Fiscal::Register *fiscalRegister);
	void test();
	virtual void proc(EventEnvelope *envelope);

private:
	enum State {
		State_Idle = 0,
		State_Register,
	};
	EventEngineInterface *eventEngine;
	Fiscal::Register *fiscalRegister;
	State state;
	uint16_t count;
	uint16_t succeedCount;
	uint16_t failedCount;
	Fiscal::Sale saleData;

	void gotoStateRegister();
	void stateRegisterEvent(EventEnvelope *envelope);
	void stateRegisterEventComplete();
	void stateRegisterEventError(EventEnvelope *envelope);
};

#endif
