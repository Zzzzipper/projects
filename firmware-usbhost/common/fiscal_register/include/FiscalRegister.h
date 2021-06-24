#ifndef COMMON_FISCALREGISTER_H
#define COMMON_FISCALREGISTER_H

#include "FiscalSale.h"
#include "config/include/ConfigEvent.h"
#include "event/include/Event2.h"
#include "event/include/EventEngine.h"
#include "utils/include/Event.h"
#include "utils/include/StringBuilder.h"
#include "mdb/MdbProtocol.h"

#define FISCAL_DECIMAL_POINT 2
#define FISCAL_ERROR_DATA_SIZE 50

namespace Fiscal {

class EventError : public EventInterface {
public:
	EventError();
	EventError(EventDeviceId deviceId);
	uint16_t code;
	StringBuilder data;
	virtual bool open(EventEnvelope *event);
	virtual bool pack(EventEnvelope *event);
};

class Register {
public:
	enum EventType {
		Event_CommandOK		= GlobalId_FiscalRegister | 0x01,
		Event_CommandError	= GlobalId_FiscalRegister | 0x02, // EventError
	};

	virtual ~Register() {}
	virtual EventDeviceId getDeviceId() = 0;
	virtual bool isRemoteFiscal() { return false; }
	virtual void reset() {}
	virtual void sale(Sale *saleData, uint32_t decimalPoint) = 0;
	virtual void getLastSale() = 0;
	virtual void closeShift() = 0;
};

class DummyRegister : public Register {
public:
	DummyRegister(EventEngineInterface *eventEngine);
	virtual EventDeviceId getDeviceId();
	virtual void reset();
	virtual void sale(Sale *saleData, uint32_t decimalPoint);
	virtual void getLastSale();
	virtual void closeShift();

private:
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;

	uint32_t calcTax(uint8_t taxRate, uint32_t profit);
};

class Context : public Mdb::DeviceContext {
public:
	Context(uint32_t masterDecimalPoint, RealTimeInterface *realtime);
	uint32_t calcTax(uint8_t taxRate, uint32_t profit);

private:
};

}

#endif
