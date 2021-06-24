#include "ticker/include/Ticker.h"
#include "platform/include/platform.h"
#include "common.h"
#include "logger/include/Logger.h"

#define TICK_SIZE 10
static Ticker *instance = NULL;

Ticker *Ticker::get() {
	if(instance == NULL) {
		instance = new Ticker();
	}
	return instance;
}

Ticker::Ticker() : consumer(NULL) {
	// Настраиваем таймер на прерывание раз в 10 млс
	OCR1A = 20000;
	TCCR1A = 0x00;
	TCCR1C = 0x00;
	TCCR1B = MASK(CS11) | MASK(WGM12); // CTC mode, CK/8
	
	// Разрешаем прерывание
	TIMSK1 = MASK(OCIE1A);
}

Ticker::~Ticker() {
	// Отключаем таймер
	TCCR1B = 0;
	// Запрещаем прерывание
	TIMSK1 = ~MASK(OCIE1A);
}

void Ticker::tick() {
	if(consumer == NULL) {
		return;
	}
	this->consumer->tick(TICK_SIZE);
}

void Ticker::registerConsumer(TickerListener *consumer) {
	this->consumer = consumer;
}

ISR(TIMER1_COMPA_vect) {
	if(instance == NULL) return;
	instance->tick();
}
