#ifndef _PINCODEDEVICE_H_
#define _PINCODEDEVICE_H_

#include "SaleManagerOrderCore.h"
#include "common/mqtt/include/MqttClient.h"

class PinCodeInterface {
public:
	virtual ~PinCodeInterface() {}
	virtual void pageStart() = 0;
	virtual void pageProgress() = 0;
	virtual void pagePinCode() = 0;
	virtual void pageSale() = 0;
	virtual void pageComplete() = 0;
};

class DummyPinCodeDevice : public PinCodeInterface {
public:
	DummyPinCodeDevice(EventEngineInterface *eventengine);
	void pageStart() override;
	void pageProgress() override;
	void pagePinCode() override;
	void pageSale() override;
	void pageComplete() override;

private:
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
};

class PinCodeDevice : public PinCodeInterface, public UartReceiveHandler {
public:
	enum EventType {
		Event_Approved	 = GlobalId_PinCode | 0x01,
		Event_Denied	 = GlobalId_PinCode | 0x02,
	};

	PinCodeDevice(TimerEngine *timerEngine, EventEngineInterface *eventengine, AbstractUart *uart);
	virtual ~PinCodeDevice();

	void pageStart() override;
	void pageProgress() override;
	void pagePinCode() override;
	void pageSale() override;
	void pageComplete() override;

	void handle() override;
    void procTimer();

private:
	enum State {
		State_Idle = 0,
		State_Start,
		State_Progress,
		State_PinCode,
		State_Sale,
		State_Complete,
	};

	EventEngineInterface *eventEngine = nullptr;
	TimerEngine *timerEngine = nullptr;
	Timer *timer = nullptr;
	AbstractUart *uart;
	EventDeviceId deviceId;
	State state;
	String pincode;

	void statePinCodeTimeout();
};

#endif // _PINCODEDEVICE_H_
