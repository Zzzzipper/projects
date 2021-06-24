#ifndef COMMON_FISCALREGISTER_TESTEVENTENGINE_H_
#define COMMON_FISCALREGISTER_TESTEVENTENGINE_H_

#include "fiscal_register/include/FiscalRegister.h"
#include "event/include/TestEventEngine.h"

class TestFiscalRegisterEventEngine : public TestEventEngine {
public:
	TestFiscalRegisterEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case Fiscal::Register::Event_CommandOK: *result << "<event=CommandOK>"; break;
		case Fiscal::Register::Event_CommandError: procEventError(envelope); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventError(EventEnvelope *envelope) {
		Fiscal::EventError event;
		if(event.open(envelope) == false) { return; }
		*result << "<event=CommandError," << event.code << ">";
	}
};

#endif
