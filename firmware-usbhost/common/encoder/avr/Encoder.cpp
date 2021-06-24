/* 
* Encoder.cpp
*
* Created: 09.07.2015 10:09:47
* Author: SSE

 Программная реализация энкодера, 2 бита, код Грея.

*/


#include "include/Encoder.h"

#define ERROR 100
static signed char states[] = {0, -1, 1, ERROR, 1, 0, ERROR, -1, -1, ERROR, 0, 1, ERROR, +1, -1, 0};

Encoder::Encoder(volatile uint8_t *abDDR, volatile uint8_t *abPORT, volatile uint8_t *abPIN, uint8_t pinA, uint8_t pinB)
: abDDR(abDDR), abPORT(abPORT), abPIN(abPIN), pinA(pinA), pinB(pinB)
{
	value = 0;
	error = 0;
	(*abDDR) &= MASK(pinA);
	(*abDDR) &= MASK(pinB);
	(*abPORT) |= MASK(pinA);
	(*abPORT) |= MASK(pinB);	
	old = readAB();
}

Encoder::~Encoder()
{
	(*abPORT) &= MASK(pinA);
	(*abPORT) &= MASK(pinB);
} 

uint8_t Encoder::readAB()
{
	uint8_t ab = (*abPIN);
	uint8_t a = ab & MASK(pinA);
	uint8_t b = ab & MASK(pinB);
	uint8_t res = 0;
	if (a) res = 1;
	if (b) res |= 2;
	return res;	
}

void Encoder::execute()
{
	uint8_t ab = readAB();
	uint8_t state = ab | (old << 2);
		
  //	state.0 = a
  //	state.1 = b
	//	state.2 = a, прошлое состояние
	//	state.3 = b, прошлое состояние
		
	signed char currentState = states[state];

	if(currentState == ERROR) {
		error++;
	} else {
		value += currentState;
		old = ab;
	}	
}

signed short Encoder::getValue()
{
	return value;
}

signed short Encoder::getAndResetValue()
{
	signed short res = value;
	value = 0;
	return res;
}

void Encoder::setValue(signed short newValue)
{
	value = newValue;
}

uint16_t Encoder::getErrorCount()
{
	return error;
}
