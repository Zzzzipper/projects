/* 
* Encoder.h
*
* Created: 09.07.2015 10:09:47
* Author: Administrator
*/


#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <stdint.h>

#include "common.h"

class Encoder
{
//variables
private:
	volatile uint8_t *abDDR;
	volatile uint8_t *abPORT;
	volatile uint8_t *abPIN;
	uint8_t pinA;
	uint8_t pinB;
	
	uint8_t old;	
	signed short value;
	uint16_t error;
	
	inline uint8_t readAB();


//functions
public:
	Encoder(volatile uint8_t *abDDR, volatile uint8_t *abPORT, volatile uint8_t *abPIN, uint8_t pinA, uint8_t pinB);
	~Encoder();
	void execute();
	signed short getValue();
	void setValue(signed short newValue);
	signed short getAndResetValue();
	uint16_t getErrorCount();
	
protected:
private:

}; //Encoder

#endif //__ENCODER_H__
