#ifndef _BEEPER_H__
#define _BEEPER_H__

#include "beeper/include/Gramophone.h"
#include "common.h"
#include "../defines.h"

class Beeper : public BeeperInterface {
public:
	static Beeper *get();

	Beeper();
	void beep(uint32_t period, uint32_t timeMs);
	virtual void initAndStart(uint32_t hz);
	virtual void stop();
private:
	void timerInit(uint32_t pulse);
	void start();
	GPIO_InitTypeDef gpio;
};

#endif
