
#ifndef _LED_H__
#define _LED_H__

#include "common.h"
#include "../defines.h"

class Led
{
public:
		static Led *get();

		Led();

		Led *setLed(int index, BYTE r, BYTE g);
		Led *setLed1(BYTE r, BYTE g);
		Led *setLed2(BYTE r, BYTE g);
		Led *setLed3(BYTE r, BYTE g);
		Led *setLed4(BYTE r, BYTE g);
		Led *allFlash(int cnt, int delay);
		Led *redFlash(int cnt, int delay);
		Led *greenFlash(int cnt, int delay);

		// ¬ыставл€ет €ркость всех светодиодов, от 0(мин) до 255(макс)
		Led *setPower(uint8_t value);
};

#endif
