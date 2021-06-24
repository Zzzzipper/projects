#ifndef COMMON_BEEPER_GRAMOPHONE_H
#define COMMON_BEEPER_GRAMOPHONE_H

#include "common/utils/include/Event.h"
#include "common/platform/include/platform.h"

class TimerEngine;
class Timer;

class BeeperInterface {
public:
	virtual void initAndStart(uint32_t hz) = 0;
	virtual void stop() = 0;
};

class Melody {
public:
	struct Note {
		uint16_t type;
		uint16_t devider;
	};

	Melody(const Note *notes, uint16_t noteNum, uint16_t noteDuration);
	uint16_t getSize();
	uint16_t getNoteType(uint16_t index);
	uint16_t getNoteDuration(uint16_t index);

private:
	const Note *notes;
	uint16_t noteNum;
	uint16_t noteDuration;
};

class MelodyElochka : public Melody {
public:
	MelodyElochka() : Melody(melody, melodySize, 2000) {}
private:
	static const uint16_t melodySize = 22;
	static const Note melody[melodySize];
};

class MelodyElochkaHalf : public Melody {
public:
	MelodyElochkaHalf() : Melody(melody, melodySize, 2000) {}
private:
	static const uint16_t melodySize = 11;
	static const Note melody[melodySize];
};

class MelodyImpireMarch : public Melody {
public:
	MelodyImpireMarch() : Melody(melody, melodySize, 2000) {}
private:
	static const uint16_t melodySize = 18;
	static const Note melody[melodySize];
};

class MelodyButton1 : public Melody {
public:
	MelodyButton1() : Melody(melody, melodySize, 2000) {}
private:
	static const uint16_t melodySize = 1;
	static const Note melody[melodySize];
};

class MelodyButton2 : public Melody {
public:
	MelodyButton2() : Melody(melody, melodySize, 2000) {}
private:
	static const uint16_t melodySize = 1;
	static const Note melody[melodySize];
};

class MelodyButton3 : public Melody {
public:
	MelodyButton3() : Melody(melody, melodySize, 2000) {}
private:
	static const uint16_t melodySize = 1;
	static const Note melody[melodySize];
};

class MelodyNfc : public Melody {
public:
	MelodyNfc() : Melody(melody, melodySize, 2000) {}
private:
	static const uint16_t melodySize = 3;
	static const Note melody[melodySize];
};

class MelodySuccess : public Melody {
public:
	MelodySuccess() : Melody(melody, melodySize, 2000) {}
private:
	static const uint16_t melodySize = 2;
	static const Note melody[melodySize];
};

class MelodyError : public Melody {
public:
	MelodyError() : Melody(melody, melodySize, 2000) {}
private:
	static const uint16_t melodySize = 4;
	static const Note melody[melodySize];
};

class GramophoneInterface {
public:
	enum EventType {
		Event_Complete = GlobalId_Gramophone | 0x01
	};
	virtual ~GramophoneInterface() {}
	virtual void play(Melody *melody, EventObserver *observer = NULL) = 0;
};

class Gramophone : public GramophoneInterface {
public:
	Gramophone(BeeperInterface *beeper, TimerEngine *timers);
	~Gramophone();
	virtual void play(Melody *melody, EventObserver *observer = NULL);
	void procTimer();

private:
	EventCourier courier;
	BeeperInterface *beeper;
	TimerEngine *timers;
	Timer *timer;
	Melody *melody;
	uint16_t step;
};

#endif
