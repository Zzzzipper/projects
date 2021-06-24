#ifndef COMMON_TIMER_ENGINE_H__
#define COMMON_TIMER_ENGINE_H__

#include "ticker/include/TickerListener.h"
#include "utils/include/List.h"
#include "Timer.h"

#define INTERRUPT_TIMERS

class TimerEngine : public TickerListener {
public:
	enum Proc {
		ProcInExecute = 0,
		ProcInTick,
	};

	TimerEngine();
	virtual ~TimerEngine();
	template<class C, void(C::*M)()>
	Timer *addTimer(C *object, Proc proc = ProcInExecute) {
		class TimerCallback : public TimerObserver {
		public:
			TimerCallback(C *context) : context(context) {}
			virtual ~TimerCallback() {}
		protected:
			C *context;
			void proc() { (this->context->*M)(); }
		};
		TimerCallback *callback = new TimerCallback(object);
		Timer *timer = new Timer(callback);
		if(timer == NULL) {
			return NULL;
		}
#ifdef INTERRUPT_TIMERS
		if(proc == ProcInExecute) {
			timers->add(timer);
		} else {
			tickTimers->add(timer);
		}
#else
		timers->add(timer);
#endif
		return timer;
	}
	void deleteTimer(Timer *timer);
	void execute();	
	virtual void tick(uint32_t tickSize);

protected:
	List<Timer> *timers;
	List<Timer> *tickTimers;
};

#endif
