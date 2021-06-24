#ifndef COMMON_BUTTONS_BUTTONENGINE_H__
#define COMMON_BUTTONS_BUTTONENGINE_H__

#include "Button.h"
#include "common/utils/include/Event.h"
#include "common/timer/include/TimerEngine.h"

class ButtonEngine {
public:
	enum Timeout {
		Timeout_Check = 100
	};

	ButtonEngine(uint16_t buttonNum);
	virtual ~ButtonEngine();
	void setObserver(EventObserver *observer);
	void init(TimerEngine* timers, uint16_t timeout = Timeout_Check);
	void check();
	void shutdown();
	bool isPressed(uint16_t buttonId);

protected:
	List<Button> *buttons;

	virtual void initButtons() = 0;
	virtual void shutdownButtons() = 0;

private:
	TimerEngine* timers;
	Timer *timer;
	uint16_t timeout;
	EventObserver *observer;

	void procTimer();

	friend class TimerEngine;
};

#endif
