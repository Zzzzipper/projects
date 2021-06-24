#include "include/Gramophone.h"

#include "common/timer/include/TimerEngine.h"
#include "common/logger/include/Logger.h"

/* ----------------------------------------------------
 * Общая теория по нотам и записи мелодий с помощью нот
 * ----------------------------------------------------
 *
 * Нота - это частота звука в герцах и время звучания этого звука.
 * Все ноты объединены в октавы.
 * Одна октава - это семь основных нот, частоты звучания которых следуют с некоторым шагом.
 * Октавы упорядочены по возрастанию частоты звучания их нот:
 *   1. субконтроктава (20,6 - 30,87)
 *   2. контроктава (32,7 - 61,74)
 *   3. большая октава (65,41 - 123,48)
 *   4. малая октава (130,82 - 246,96)
 *   5. первая октава (261,63 - 493,88)
 *   6. вторая октава (523,25 - 987,75)
 *   7. третья октава (1046,5 - 1975,5)
 *   8. четвертая октава (2093 - 3951)
 *   9. пятая октава (4186 - 5274)
 *
 * Для каждой мелодии нужно знать основную октаву и длительность звучания ноты.
 * В самой мелодии для каждой ноты указывают её расположение относительно основной октавы
 * и делитель, на который делится длительность звучания ноты.
 *
 * Например, основая октава первая и нужно задать ноту "соль четверть", то параметры
 * ноты будут следующими: Note_O1_So и делитель 4.
 *
 * Все мелодии пишутся относительно основной октавы. В параметрах ноты иногда пишут
 * нижи или выше октавы.
 * Например, для задания фрагмента мелодии с основной первой октавой (O1):
 *   ля бемоль четверть,
 *   фа нижняя восьмая с шестнадцатой,
 *   до верхняя шестнадцатая,
 * Необходимо задать:
 *   Note_O1_La, 4
 *   Note_OS_Fa, 5   октава ниже O1 - это малая октава (OctaveSmall)
 *   Note_O2_Do, 16  октава выше O1 - это O2 (Octave 2)
 *
 * Есть еще хитрые ноты: бимоль и диез.
 * Нота бимоль - имеет частоту звучания ровно по середине между текущей нотой и предыдущей.
 * Например, ля бимоль первой октавы будет иметь частоту 416, когда ля - 440, а соль - 392.
 * Нота диез - имеет частоту звучния ровоно по середине между текущей нотой и следующей.
 * Например, соль диез первой октавы будет иметь частоту 416, когда ля - 440, а соль - 392.
 * Да, вы правильно поняли, ля бимоль и соль диез означают одну и туже ноту. Гуманитарии...
 *
 * Частоты нот http://www.gitaristam.ru/school/frequency.htm
 *
 */
enum Note {
	// малая октава
	Note_OS_Do = 131,
	Note_OS_Re = 147,
	Note_OS_Mi = 165,
	Note_OS_Fa = 176,
	Note_OS_So = 196,
	Note_OS_La = 220,
	Note_OS_Si = 247,

	// первая октава
	Note_O1_Do = 262,
	Note_O1_Re = 294,
	Note_O1_Mi = 330,
	Note_O1_Fa = 349,
	Note_O1_So = 392,
	Note_O1_LaBemol = 416,
	Note_O1_La = 440,
	Note_O1_Si = 494,

	// вторая октава
	Note_O2_Do = 523,
	Note_O2_Re = 587,
	Note_O2_Mi = 659,
	Note_O2_Fa = 698,
	Note_O2_So = 784,
	Note_O2_La = 880,
	Note_O2_Si = 988,
};

const Melody::Note MelodyElochka::melody[melodySize] = {
	{ Note_O1_So, 4 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_So, 4 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_So, 8 },
	{ Note_O1_Fa, 8 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_Re, 8 },
	{ Note_O1_Do, 2 },

	{ Note_O1_La, 4 },
	{ Note_O2_Do, 8 },
	{ Note_O1_La, 8 },
	{ Note_O1_So, 4 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_So, 8 },
	{ Note_O1_Fa, 8 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_Re, 8 },
	{ Note_O1_Do, 2 }
};

const Melody::Note MelodyElochkaHalf::melody[melodySize] = {
	{ Note_O1_So, 4 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_So, 4 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_So, 8 },
	{ Note_O1_Fa, 8 },
	{ Note_O1_Mi, 8 },
	{ Note_O1_Re, 8 },
	{ Note_O1_Do, 2 }
};

const Melody::Note MelodyImpireMarch::melody[melodySize] = {
	{ Note_O1_La, 4 },
	{ Note_O1_La, 4 },
	{ Note_O1_La, 4 },
	{ Note_O1_Fa, 5 },
	{ Note_O2_Do, 16 },
	{ Note_O1_La, 4 },
	{ Note_O1_Fa, 5 },
	{ Note_O2_Do, 16 },
	{ Note_O1_La, 2 },

	{ Note_O2_Mi, 4 },
	{ Note_O1_Mi, 4 },
	{ Note_O1_Mi, 4 },
	{ Note_O2_Fa, 5 },
	{ Note_O2_Do, 16 },
	{ Note_O1_LaBemol, 4 },
	{ Note_O1_Fa, 5 },
	{ Note_O2_Do, 16 },
	{ Note_O1_La, 2 }
};

const Melody::Note MelodyButton1::melody[melodySize] = {
	{ Note_O1_Do, 16 },
};

const Melody::Note MelodyButton2::melody[melodySize] = {
	{ Note_O1_Re, 16 },
};

const Melody::Note MelodyButton3::melody[melodySize] = {
	{ Note_O1_Mi, 16 },
};

const Melody::Note MelodyNfc::melody[melodySize] = {
	{ Note_O1_Do, 16 },
	{ Note_O1_Re, 16 },
	{ Note_O1_Mi, 16 },
};

const Melody::Note MelodySuccess::melody[melodySize] = {
	{ Note_O1_Do, 4 },
	{ Note_O1_Re, 8 }
};

const Melody::Note MelodyError::melody[melodySize] = {
	{ Note_O1_Do, 2 },
	{ Note_O1_Do, 2 },
	{ Note_O1_Do, 2 },
	{ Note_O1_Do, 8 }
};

Melody::Melody(const Note *notes, uint16_t noteNum, uint16_t noteDuration) :
	notes(notes),
	noteNum(noteNum),
	noteDuration(noteDuration)
{}

uint16_t Melody::getSize() {
	return noteNum;
}

uint16_t Melody::getNoteType(uint16_t index) {
	if(index >= noteNum) {
		return Note_O1_Do;
	}
	return notes[index].type;
}

uint16_t Melody::getNoteDuration(uint16_t index) {
	if(index >= noteNum) {
		return noteDuration;
	}
	return (noteDuration / notes[index].devider);
}

Gramophone::Gramophone(BeeperInterface *beeper, TimerEngine *timers) :
	beeper(beeper),
	timers(timers)
{
	this->timers = timers;
	this->timer = timers->addTimer<Gramophone, &Gramophone::procTimer>(this, TimerEngine::ProcInTick);
}

Gramophone::~Gramophone() {
	timers->deleteTimer(this->timer);
}

void Gramophone::play(Melody *melody, EventObserver *observer) {
	this->melody = melody;
	courier.setRecipient(observer);
	step = 0;
	timer->start(1);
}

void Gramophone::procTimer() {
	if(step < melody->getSize()) {
		beeper->stop();
		beeper->initAndStart(melody->getNoteType(step));
		timer->start(melody->getNoteDuration(step));
		step++;
	} else {
		beeper->stop();
		Event event(Event_Complete);
		courier.deliver(&event);
	}
}
