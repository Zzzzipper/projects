#include "include/TimeMeter.h"

#include "timer/stm32/include/SystemTimer.h"

MicroSecondMeter::MicroSecondMeter() {
	st = SystemTimer::get();
	maxValue = st->getUsMax();
	start();
}

void MicroSecondMeter::start() {
	from = st->getUs();
}

void MicroSecondMeter::meter() {
	to = st->getUs();
	if(from <= to) {
		dif = to - from;
	} else {
		dif = to + (maxValue - from);
		LOG_DEBUG(LOG_TM, ">>>TOOBIG=" << from << "," << to);
	}
	from = to;
}

/*
 * «апускает подсчет.
 * ѕараметры:
 * cycleNumber - количество измерений, прежде чем будет выведены средние показатели в логи
 * –екомендуетс€ от нескольких сотен, до нескольких тыс€ч.
 * ћаксимальное врем€ между вызовами start и meter или meter и meter не должно быть больше
 * 0xFFFFFFFF / (SystemCoreClock/1000000). ѕри частоте контроллера 168ћгц это примерно 25 секунд.
 */
void MicroCycleMeter::start(uint32_t cycleNumber) {
	cntNumber = cycleNumber;
	cnt = 0;
	cycleMaxTime = 0;
	globalMaxTime = 0;
	meter.start();
}

/*
 * «начени€ полей в лог-сообщении:
 * cmean - среднее арифметическое одного цикла
 * ctime - общее врем€ выполнени€ заданного количества циклов
 * cmax - максимальное врем€ выполнени€ одного цикла в одном круге измерений
 * gmax - максимальное врем€ выполнени€ одного цикла за все круги измерений
 */
void MicroCycleMeter::cycle() {
	meter.meter();
	uint32_t dif = meter.getDif();
	if(dif > cycleMaxTime) { cycleMaxTime = dif; }
	if(dif > globalMaxTime) { globalMaxTime = dif; }
	time += dif;
	cnt++;
	if(cnt == cntNumber) {
		LOG_INFO(LOG_TM, ">>>cmean=" << time/cntNumber << ", ctime=" << time << ", cmax=" << cycleMaxTime << ", gmax=" << globalMaxTime);
		cycleMaxTime = 0;
		cnt = 0;
		time = 0;
	}
}


static MicroIntervalMeter *instance = 0;

MicroIntervalMeter *MicroIntervalMeter::get() {
	if(instance) return instance;
	return new MicroIntervalMeter();
}

MicroIntervalMeter::MicroIntervalMeter() {
	instance = this;
	start(0);
}

void MicroIntervalMeter::start(uint32_t maxIntervalSize) {
	this->maxIntervalSize = maxIntervalSize;
	meter.start();
}

bool MicroIntervalMeter::check() {
	meter.meter();
	return (meter.getDif() < maxIntervalSize);
}
