#include "PinCodeDevice.h"

#include "common/logger/include/Logger.h"

DummyPinCodeDevice::DummyPinCodeDevice(EventEngineInterface *eventEngine) :
	eventEngine(eventEngine),
	deviceId(eventEngine)
{
}

void DummyPinCodeDevice::pageStart() {
	LOG(">>>>>>>>>>pageStart");
}

void DummyPinCodeDevice::pageProgress() {
	LOG(">>>>>>>>>>pageProgress");
}

void DummyPinCodeDevice::pagePinCode() {
	LOG(">>>>>>>>>>pagePinCode");
	OrderDeviceInterface::EventPinCodeCompleted event(deviceId, "1234");
	eventEngine->transmit(&event);
}

void DummyPinCodeDevice::pageSale() {
	LOG(">>>>>>>>>>pageSale");
}

void DummyPinCodeDevice::pageComplete() {
	LOG(">>>>>>>>>>pageComplete");
}

PinCodeDevice::PinCodeDevice(
	TimerEngine *timerEngine,
	EventEngineInterface *eventEngine,
	AbstractUart *uart
) :
	eventEngine(eventEngine),
	timerEngine(timerEngine),
	uart(uart),
	deviceId(eventEngine),
	state(State_Idle),
	pincode(PINCODE_SIZE, PINCODE_SIZE)
{
	uart->setReceiveHandler(this);
	timer = timerEngine->addTimer<PinCodeDevice, &PinCodeDevice::procTimer>(this);
}

PinCodeDevice::~PinCodeDevice() {
}

void PinCodeDevice::pageStart() {
	LOG_DEBUG(LOG_SM, "pageStart");
	uart->send('p');
	uart->send('a');
	uart->send('g');
	uart->send('e');
	uart->send(' ');
	uart->send('0');
	uart->send(0xFF);
	uart->send(0xFF);
	uart->send(0xFF);
	state = State_Start;
}

void PinCodeDevice::pageProgress() {
	LOG_DEBUG(LOG_SM, "pageProgress");
	uart->send('p');
	uart->send('a');
	uart->send('g');
	uart->send('e');
	uart->send(' ');
	uart->send('1');
	uart->send(0xFF);
	uart->send(0xFF);
	uart->send(0xFF);
	state = State_Progress;
}

void PinCodeDevice::pagePinCode() {
	LOG_DEBUG(LOG_SM, "pagePinCode");
	uart->send('p');
	uart->send('a');
	uart->send('g');
	uart->send('e');
	uart->send(' ');
	uart->send('2');
	uart->send(0xFF);
	uart->send(0xFF);
	uart->send(0xFF);
	timer->start(60000);
	state = State_PinCode;
}

void PinCodeDevice::pageSale() {
	LOG_DEBUG(LOG_SM, "pageSale");
	uart->send('p');
	uart->send('a');
	uart->send('g');
	uart->send('e');
	uart->send(' ');
	uart->send('3');
	uart->send(0xFF);
	uart->send(0xFF);
	uart->send(0xFF);
	state = State_Sale;
}

void PinCodeDevice::pageComplete() {
	LOG_DEBUG(LOG_SM, "pageComplete");
	uart->send('p');
	uart->send('a');
	uart->send('g');
	uart->send('e');
	uart->send(' ');
	uart->send('4');
	uart->send(0xFF);
	uart->send(0xFF);
	uart->send(0xFF);
	state = State_Complete;
}

/*
SXXXXE
SCCCCE
 */
void PinCodeDevice::handle() {
	while(uart->isEmptyReceiveBuffer() == false) {
		uint8_t b = uart->receive();
		if(b == 'S') {
			pincode.clear();
		}
		if(b >= 0x30 && b <= 0x39) {
			pincode.add(b);
		}
		if(b == 'E') {
			timer->stop();
			this->pageProgress();
			state = State_Idle;
			if(pincode.getLen() <= 0 || pincode[0] == 'C') {
				EventInterface event(deviceId, OrderDeviceInterface::Event_PinCodeCancelled);
				eventEngine->transmit(&event);
				return;
			} else {
				OrderDeviceInterface::EventPinCodeCompleted event(deviceId, pincode.getString());
				eventEngine->transmit(&event);
				return;
			}
		}
	}
}

void PinCodeDevice::procTimer() {
	LOG_DEBUG(LOG_SM, "procTimer");
	switch(state) {
	case State_PinCode: statePinCodeTimeout(); return;
	default: LOG_ERROR(LOG_SM, "Unwaited timeout " << state);
	}
}

void PinCodeDevice::statePinCodeTimeout() {
	LOG_DEBUG(LOG_SM, "statePinCodeTimeout");
	state = State_Idle;
	EventInterface event(deviceId, OrderDeviceInterface::Event_PinCodeCancelled);
	eventEngine->transmit(&event);
}
