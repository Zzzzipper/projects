#include "common/beeper/include/AlarmBeeper.h"

#include "common/timer/include/TimerEngine.h"

#include "lib/utils/stm32/Beeper.h"

AlarmBeeper::AlarmBeeper(TimerEngine *timers) {
	this->timers = timers;
	this->timer = timers->addTimer<AlarmBeeper, &AlarmBeeper::procTimer>(this);
}

AlarmBeeper::~AlarmBeeper() {
	timers->deleteTimer(timer);
}

void AlarmBeeper::start() {
	beep = true;
	Beeper::get()->stop();
	Beeper::get()->initAndStart(262);
	timer->start(500);
}

void AlarmBeeper::stop() {
	timer->stop();
}

void AlarmBeeper::procTimer() {
	if(beep == true) {
		Beeper::get()->stop();
		timer->start(2000);
		beep = false;
	} else {
		Beeper::get()->initAndStart(262);
		timer->start(500);
		beep = true;
	}
}
