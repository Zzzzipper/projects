#ifndef COMMON_UTILS_EVENT_H_
#define COMMON_UTILS_EVENT_H_

#include "StringBuilder.h"

#include <stdint.h>
#include <stddef.h>

enum GlobalId {
	GlobalId_SlaveBillValidator		= 0x0100,
	GlobalId_SlaveCoinChanger		= 0x0200,
	GlobalId_SlaveCashless1			= 0x0300,
	GlobalId_MasterBillValidator	= 0x0400,
	GlobalId_MasterCoinChanger		= 0x0500,
	GlobalId_MasterCashless			= 0x0600,
	GlobalId_Keyboard				= 0x0700,
	GlobalId_LoadController			= 0x0800,
	GlobalId_Buttons				= 0x0900,
	GlobalId_Detector				= 0x0A00,
	GlobalId_I2C					= 0x0B00,
	GlobalId_Sim900					= 0x0C00,
	GlobalId_TcpIp					= 0x0D00,
	GlobalId_HttpClient				= 0x0E00,
	GlobalId_FiscalRegister			= 0x0F00,
	GlobalId_Executive				= 0x1000,
	GlobalId_Dex					= 0x1100,
	GlobalId_Config					= 0x1200,
	GlobalId_Gramophone				= 0x1300,
	GlobalId_Ecp					= 0x1400,
	GlobalId_DataParser				= 0x1500,
	GlobalId_SnifferBillValidator	= 0x1600,
	GlobalId_SnifferCoinChanger		= 0x1700,
	GlobalId_SnifferCashless		= 0x1800,
	GlobalId_SaleManager			= 0x1900,
	GlobalId_EventRegistrar			= 0x1A00,
	GlobalId_System					= 0x1B00,
	GlobalId_Rfid					= 0x1C00,
	GlobalId_SlaveComGateway		= 0x1D00,
	GlobalId_Battery				= 0x1E00,
	GlobalId_CodeScanner			= 0x1F00,
	GlobalId_Order					= 0x2000,
	GlobalId_OrderMaster			= 0x2100,
	GlobalId_OrderDevice			= 0x2200,
	GlobalId_DialogDirect			= 0x2300,
	GlobalId_MqttClient				= 0x2400,
	GlobalId_PinCode				= 0x2500,
	GlobalId_GroupList				= 0x2600,
	GlobalId_SaleList				= 0x2700,
	GlobalId_SaleWizard				= 0x2800,
	GlobalId_ScreenSaver			= 0x2900,
	GlobalId_QuickPay				= 0x2A00,
};

enum SystemEvent {
	SystemEvent_Reboot = GlobalId_System | 0x01,
};

#define EVENT_DEVICE_MASK 0xFF00
#define EVENT_EVENT_MASK 0x00FF

class Event {
public:
	Event() {}
	Event(uint16_t type) : type(type) {}
	Event(uint16_t type, uint8_t value) : type(type), b1(value) {}
	Event(uint16_t type, uint16_t value) : type(type), b2(value) {}
	Event(uint16_t type, uint32_t value) : type(type), b4(value) {}
	void set(uint16_t type) { this->type = type; }
	void setUint8(uint16_t type, uint8_t value) { this->type = type; b1 = value; }
	void setUint16(uint16_t type, uint16_t value) { this->type = type; b2 = value; }
	void setUint32(uint16_t type, uint32_t value) { this->type = type; b4 = value; }
	uint16_t getType() const { return type; }
	uint16_t getDeviceId() const { return type & EVENT_DEVICE_MASK; }
	uint16_t getEventId() const { return type & EVENT_EVENT_MASK; }
	uint8_t  getUint8() const { return b1; }
	uint16_t getUint16() const { return b2; }
	uint32_t getUint32() const { return b4; }

private:
	uint16_t type;
	union {
		uint8_t b1;
		uint16_t b2;
		uint32_t b4;
	};
};

#define EVENT_TRACE_SIZE 40

class EventError : public Event {
public:
	StringBuilder trace;

	EventError(uint16_t type) : Event(type), trace(EVENT_TRACE_SIZE, EVENT_TRACE_SIZE) {}
	EventError(uint16_t type, const char *prev) : Event(type), trace(EVENT_TRACE_SIZE, EVENT_TRACE_SIZE) {
		if(prev != NULL) { trace << prev; }
	}
	EventError(uint16_t type, const char *name, uint32_t line, const char *prev = NULL) : Event(type), trace(EVENT_TRACE_SIZE, EVENT_TRACE_SIZE) {
		trace << name << line;
		if(prev != NULL) { trace << ";" << prev; }
	}
};

class EventObserver {
public:
	virtual ~EventObserver() {}
	virtual void proc(Event *) = 0;
};

class EventCourier {
public:
	EventCourier() : observer(NULL) {}
	EventCourier(EventObserver *observer) : observer(observer) {}
	void setRecipient(EventObserver *observer) { this->observer = observer;	}
	void deliver(Event *event) { if(observer != NULL) { observer->proc(event); } }
	void deliver(uint16_t type) { Event event(type); deliver(&event); }
	bool hasRecipient() { return (observer != NULL); }
private:
	EventObserver *observer;
};

#endif
