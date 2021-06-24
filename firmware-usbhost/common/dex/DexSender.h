#ifndef COMMON_DEX_SENDER_H_
#define COMMON_DEX_SENDER_H_

#include <stdint.h>

class AbstractUart;
class TimerEngine;
class Timer;

namespace Dex {

class Sender {
public:
	Sender();
	void init(AbstractUart *uart, TimerEngine *timers);
	void sendControl(uint8_t control);
	void sendConfirm();
    void repeatConfirm();
	void sendData(const void *data, const uint16_t len, const uint8_t control1, const uint8_t control2);
	void resetParity() { parity = 0; }
	void procTimer();

protected:
	enum Type {
		Type_Control = 0,
		Type_Confirm,
		Type_Data
	};

	AbstractUart *uart;
	Timer *timer;
	Type messageType;
	const void *data;
	uint16_t dataLen;
	uint8_t control1;
	uint8_t control2;
	uint8_t parity;

	void sendControl2();
	void sendConfirm2();
	void sendData2();
};

}

#endif
