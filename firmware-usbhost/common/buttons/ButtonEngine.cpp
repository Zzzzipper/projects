#include "include/ButtonEngine.h"
#include "logger/include/Logger.h"

ButtonEngine::ButtonEngine(uint16_t buttonNum) : timers(NULL), timer(NULL), observer(NULL) {
	buttons = new List<Button>(buttonNum);
}

// в деструкторе нельзя вызывать виртуальные методы!
ButtonEngine::~ButtonEngine() {
	if(timer != NULL) {
		timers->deleteTimer(timer);
		timer = NULL;
	}
	delete buttons;
}

void ButtonEngine::setObserver(EventObserver *observer) {
	this->observer = observer;
}

void ButtonEngine::init(TimerEngine* timers, uint16_t timeout) {
	shutdown();
	initButtons();
	this->timers = timers;
	this->timeout = timeout;
	this->timer = timers->addTimer<ButtonEngine, &ButtonEngine::procTimer>(this);
	this->timer->start(this->timeout);
}

void ButtonEngine::check() {
	for(uint16_t i = 0; i < buttons->getSize(); i++) {
		Button *button = buttons->get(i);
		if(button->isChange() == true) {
			if(observer != NULL) {
				uint8_t status = button->isPressed();
				Event event(button->getId(), status);
				observer->proc(&event);
			}
		}
	}
}

void ButtonEngine::shutdown() {
	if(timer != NULL) {
		timers->deleteTimer(timer);
		timer = NULL;
	}
	shutdownButtons();
}

void ButtonEngine::procTimer() {
	check();
	timer->start(timeout);
}

bool ButtonEngine::isPressed(uint16_t buttonId) {
	for(uint16_t i = 0; i < buttons->getSize(); i++) {
		Button *button = buttons->get(i);
		if(button->getId() == buttonId) {
			return button->isPressed();
		}
	}
	return false;
}
