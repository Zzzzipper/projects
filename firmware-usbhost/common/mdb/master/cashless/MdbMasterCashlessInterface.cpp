#include "MdbMasterCashlessInterface.h"

#include "fiscal_register/include/FiscalSale.h"
#include "logger/RemoteLogger.h"
#include "logger/include/Logger.h"

#include <string.h>

enum EventDepositeParam {
	EventDepositeParam_Type1 = 0,
	EventDepositeParam_Nominal1,
	EventDepositeParam_Type2,
	EventDepositeParam_Nominal2,
};

MdbMasterCashlessInterface::EventApproved::EventApproved() : EventInterface(Event_VendApproved) {}

MdbMasterCashlessInterface::EventApproved::EventApproved(EventDeviceId deviceId) : EventInterface(deviceId, Event_VendApproved) {}

MdbMasterCashlessInterface::EventApproved::EventApproved(EventDeviceId deviceId, uint8_t type1, uint32_t nominal1) :
	EventInterface(deviceId, Event_VendApproved),
	type1(type1),
	value1(nominal1),
	type2(0),
	value2(0)
{
}

MdbMasterCashlessInterface::EventApproved::EventApproved(EventDeviceId deviceId, uint8_t type1, uint32_t nominal1, uint8_t type2, uint32_t nominal2) :
	EventInterface(deviceId, Event_VendApproved),
	type1(type1),
	value1(nominal1),
	type2(type2),
	value2(nominal2)
{
}

void MdbMasterCashlessInterface::EventApproved::set(uint8_t type1, uint32_t nominal1, uint8_t type2, uint32_t nominal2) {
	this->type1 = type1;
	this->value1 = nominal1;
	this->type2 = type2;
	this->value2 = nominal2;
}

bool MdbMasterCashlessInterface::EventApproved::open(EventEnvelope *envelope) {
	if(EventInterface::open(envelope) == false) { return false; }
	if(envelope->getUint8(EventDepositeParam_Type1, &type1) == false) { return false; }
	if(envelope->getUint32(EventDepositeParam_Nominal1, &value1) == false) { return false; }
	if(envelope->getUint8(EventDepositeParam_Type2, &type2) == false) { return false; }
	if(envelope->getUint32(EventDepositeParam_Nominal2, &value2) == false) { return false; }
	return true;
}

bool MdbMasterCashlessInterface::EventApproved::pack(EventEnvelope *envelope) {
	if(EventInterface::pack(envelope) == false) { return false; }
	if(envelope->addUint8(EventDepositeParam_Type1, type1) == false) { return false; }
	if(envelope->addUint32(EventDepositeParam_Nominal1, value1) == false) { return false; }
	if(envelope->addUint8(EventDepositeParam_Type2, type2) == false) { return false; }
	if(envelope->addUint32(EventDepositeParam_Nominal2, value2) == false) { return false; }
	return true;
}
