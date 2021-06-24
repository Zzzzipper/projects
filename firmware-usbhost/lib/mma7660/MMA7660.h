
#ifndef __MMA7660_H_
#define __MMA7660_H_

#include <stdint.h>
#include "common/uart/stm32/include/uart.h"
#include "common/utils/include/StringBuilder.h"
#include "common/utils/include/Event.h"

class MMA7660: public EventObserver
{
public:
	static MMA7660 *get();

	struct Values {
		signed char x;
		signed char y;
		signed char z;

		Values() {
			x = 0;
			y = 0;
			z = 0;
		}
	};
	virtual void proc(Event *);

private:
	signed char asyncData[3];

	bool initialized;
	Uart *asyncPrintUart;

	MMA7660();

	bool startTransmission(uint8_t transmissionDirection);
	void writeData(uint8_t data);
//	uint8_t readData();
	void stopTransmission();
	struct Values getValues(signed char data[]);


public:

	bool init();
	bool isInitialized();
	void syncPrint(Uart *uart);
	void asyncPrint(Uart *uart);
	struct Values asyncReadValues();

};


#endif
