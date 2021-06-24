#include "FiscalTest.h"

#include "common/logger/include/Logger.h"

#ifdef DEBUG
#define CHECK_NUMBER 50
#else
#define CHECK_NUMBER 1
#endif

FiscalTest::FiscalTest(EventEngineInterface *eventEngine, Fiscal::Register *fiscalRegister) :
	eventEngine(eventEngine),
	fiscalRegister(fiscalRegister),
	state(State_Idle)
{
	LOG_DEBUG(LOG_MODEM, "subscribe to " << fiscalRegister->getDeviceId().getValue());
	eventEngine->subscribe(this, GlobalId_FiscalRegister, fiscalRegister->getDeviceId());
}

void FiscalTest::test() {
	LOG_INFO(LOG_MODEM, "test");
	if(fiscalRegister == NULL) {
		LOG_ERROR(LOG_MODEM, "Fiscal register not defined");
		return;
	}
	if(state != State_Idle) {
		LOG_ERROR(LOG_MODEM, "Wrong state " << state);
		return;
	}
	count = 0;
	succeedCount = 0;
	failedCount = 0;
	gotoStateRegister();
}

void FiscalTest::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_MODEM, "proc " << state << "," << envelope->getType() << "," << envelope->getDevice());
	switch(state) {
		case State_Idle: LOG_DEBUG(LOG_SM, "Ignore event " << state << "," << envelope->getType()); return;
		case State_Register: stateRegisterEvent(envelope); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << state << "," << envelope->getType());
	}
}

void FiscalTest::gotoStateRegister() {
	StringBuilder name;
	name << "Тесточино" << count;
	saleData.setProduct("", 0, name.getString(), 200, Fiscal::TaxRate_NDS10, 1);
	saleData.paymentType = Fiscal::Payment_Cash;
	saleData.credit = 200;
	fiscalRegister->sale(&saleData, 2);
	state = State_Register;
}

void FiscalTest::stateRegisterEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_MODEM, "statePrintEvent");
	switch(envelope->getType()) {
		case Fiscal::Register::Event_CommandOK: stateRegisterEventComplete(); return;
		case Fiscal::Register::Event_CommandError: stateRegisterEventError(envelope); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << state << "," << envelope->getType());
	}
}

void FiscalTest::stateRegisterEventComplete() {
	LOG_DEBUG(LOG_MODEM, "statePrintEventComplete");
	LOG_INFO(LOG_MODEM, ">>>>>>>>>>>>>>>> ROUND" << count << " SUCCEED");
	count++;
	succeedCount++;
	if(count >= CHECK_NUMBER) {
		LOG_INFO(LOG_MODEM, ">>>>>>>>>>>>>>>> ALL CHECK COMPLETE " << succeedCount << "/" << count);
		state = State_Idle;
		return;
	}
	gotoStateRegister();
}

void FiscalTest::stateRegisterEventError(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_MODEM, "statePrintEventError");
	Fiscal::EventError event(fiscalRegister->getDeviceId());
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_MODEM, "Event open failed");
		state = State_Idle;
		return;
	}
	LOG_ERROR(LOG_MODEM, "Check print error: code=" << event.code << ",data=" << event.data.getString());
	LOG_INFO(LOG_MODEM, ">>>>>>>>>>>>>>>> ROUND" << count << " FAILED");
	state = State_Idle;
}
