#ifndef COMMON_BEEPER_ALARMBEEPER_H
#define COMMON_BEEPER_ALARMBEEPER_H

class TimerEngine;
class Timer;

class AlarmBeeper {
public:
	AlarmBeeper(TimerEngine *timers);
	~AlarmBeeper();
	void start();
	void stop();
	void procTimer();

private:
	TimerEngine *timers;
	Timer *timer;
	bool beep;
};

#endif
