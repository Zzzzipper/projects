#ifndef COMMON_TEST_CASHLESSEVENTENGINE_H
#define COMMON_TEST_CASHLESSEVENTENGINE_H

#include "mdb/master/cashless/MdbMasterCashless.h"
#include "event/include/TestEventEngine.h"

class TestCashlessEventEngine : public TestEventEngine {
public:
	TestCashlessEventEngine(StringBuilder *result) : TestEventEngine(result) {}
	virtual bool transmit(EventEnvelope *envelope) {
		switch(envelope->getType()) {
		case MdbMasterCashless::Event_Ready: procEvent(envelope, MdbMasterCashless::Event_Ready, "Ready"); break;
		case MdbMasterCashless::Event_Error: procEventUint16(envelope, MdbMasterCashless::Event_Error, "Error"); break;
		case MdbMasterCashless::Event_SessionBegin: procEventUint32(envelope, MdbMasterCashless::Event_SessionBegin, "SessionBegin"); break;
		case MdbMasterCashless::Event_RevalueApproved: procEvent(envelope, MdbMasterCashless::Event_RevalueApproved, "RevalueApproved"); break;
		case MdbMasterCashless::Event_RevalueDenied: procEvent(envelope, MdbMasterCashless::Event_RevalueDenied, "RevalueDenied"); break;
		case MdbMasterCashless::Event_VendApproved: procEventApproved(envelope, MdbMasterCashless::Event_VendApproved, "VendApproved"); break;
		case MdbMasterCashless::Event_VendDenied: procEvent(envelope, MdbMasterCashless::Event_VendDenied, "VendDenied"); break;
		case MdbMasterCashless::Event_SessionEnd: procEvent(envelope, MdbMasterCashless::Event_SessionEnd, "SessionEnd"); break;
		default: TestEventEngine::transmit(envelope);
		}
		return true;
	}

private:
	void procEventApproved(EventEnvelope *envelope, uint16_t type, const char *name) {
		MdbMasterCashlessInterface::EventApproved event;
		if(envelope->getType() != type) { *result << "<event-unwaited-type>"; return; }
		if(event.open(envelope) == false) { return; }
		*result << "<event=" << event.getDevice() << "," << name << "," << event.getType1() << "," << event.getValue1() << "," << event.getType2() << "," << event.getValue2() << ">";
	}
};

#endif
