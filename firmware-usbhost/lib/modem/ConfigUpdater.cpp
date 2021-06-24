#include "ConfigUpdater.h"

#include "common/logger/include/Logger.h"

#define STEP_SIZE 100
#define REBOOT_DELAY 500

ConfigUpdater::ConfigUpdater(TimerEngine *timers, ConfigModem *config, EventObserver *observer) :
	config(config),
	timers(timers),
	courier(observer),
	state(State_Idle),
	parser(config)
{
	this->timer = timers->addTimer<ConfigUpdater, &ConfigUpdater::procTimer>(this);
}

ConfigUpdater::~ConfigUpdater() {
	timers->deleteTimer(timer);
}

bool ConfigUpdater::checkCrc(StringBuilder *data) {
	checker.start();
	checker.procData(data->getData(), data->getLen());
	checker.complete();
	return (checker.hasError() == false);
}

void ConfigUpdater::resize(StringBuilder *data, bool fixedDecimalPoint) {
	LOG_DEBUG(LOG_MODEM, "resize");
	result = Dex::DataParser::Result_Busy;
	this->data = data;
	this->fixedDecimalPoint = fixedDecimalPoint;
	gotoStateInit();
}

void ConfigUpdater::update(StringBuilder *data, bool fixedDecimalPoint) {
	LOG_DEBUG(LOG_MODEM, "update");
	result = Dex::DataParser::Result_Busy;
	this->data = data;
	this->fixedDecimalPoint = fixedDecimalPoint;
	gotoStateUpdate();
}

Dex::DataParser::Result ConfigUpdater::getResult() {
	return result;
}

void ConfigUpdater::procTimer() {
	LOG_DEBUG(LOG_MODEM, "procTimer");
	switch(state) {
	case State_Init: stateInitTimeout(); break;
	case State_InitLast: stateInitLastTimeout(); break;
	case State_Compare: stateCompareTimeout(); break;
	case State_Resize: stateResizeTimeout(); break;
	case State_Update: stateUpdateTimeout(); break;
	case State_UpdateLast: stateUpdateLastTimeout(); break;
	default: LOG_ERROR(LOG_MODEM, "Unwaited timeout " << state);
	}
}

void ConfigUpdater::gotoStateInit() {
	initer.start();
	timer->start(1);
	offset = 0;
	stepSize = STEP_SIZE;
	if((offset + stepSize) < data->getLen()) {
		state = State_Init;
	} else {
		stepSize = data->getLen() - offset;
		state = State_InitLast;
	}
}

void ConfigUpdater::stateInitTimeout() {
	LOG_DEBUG(LOG_MODEM, "stateCompareTimeout");
	initer.procData(data->getData() + offset, stepSize);
	timer->start(1);
	offset += stepSize;
	stepSize = STEP_SIZE;
	if((offset + stepSize) >= data->getLen()) {
		stepSize = data->getLen() - offset;
		state = State_InitLast;
	}
}

void ConfigUpdater::stateInitLastTimeout() {
	LOG_DEBUG(LOG_MODEM, "stateCompareLastTimeout");
	initer.procData(data->getData() + offset, stepSize);
	initer.complete();
	if(initer.hasError() == true) {
		LOG_ERROR(LOG_MODEM, "Config parsing failed.");
		result = Dex::DataParser::Result_Error;
		courier.deliver(Dex::DataParser::Event_AsyncError);
		return;
	}

	if(config->isResizeable(initer.getPrices(), initer.getProducts()) == false) {
		gotoStateUpdate();
	} else {
		gotoStateCompare();
	}
}

void ConfigUpdater::gotoStateCompare() {
	LOG_INFO(LOG_MODEM, "gotoStateComparePlanogram");
	config->asyncComparePlanogramStart(initer.getPrices(), initer.getProducts());
	timer->start(1);
	state = State_Compare;
}

void ConfigUpdater::stateCompareTimeout() {
	LOG_DEBUG(LOG_MODEM, "stateComparePlanogramTimeout");
	if(config->asyncComparePlanogramProc() == true) {
		timer->start(1);
		return;
	}

	if(config->asyncComparePlanogramResult() == Evadts::Result_OK) {
		LOG_INFO(LOG_MODEM, "Config not need init.");
		gotoStateUpdate();
		return;
	}

	LOG_INFO(LOG_MODEM, "Config need init.");
	gotoStateResize();
}

void ConfigUpdater::gotoStateResize() {
	LOG_DEBUG(LOG_MODEM, "gotoStateResize");
	if(config->asyncResizePlanogramStart(initer.getPrices(), initer.getProducts()) == false) {
		LOG_ERROR(LOG_MODEM, "Wrong resize data.");
		result = Dex::DataParser::Result_Error;
		courier.deliver(Dex::DataParser::Event_AsyncError);
		return;
	}
	timer->start(1);
	state = State_Resize;
}

void ConfigUpdater::stateResizeTimeout() {
	LOG_DEBUG(LOG_MODEM, "stateResizeTimeout");
	if(config->asyncResizePlanogramProc() == true) {
		timer->start(1);
		return;
	}

	config->getAutomat()->setConfigId(0);
	config->getAutomat()->save();
	gotoStateUpdate();
}

void ConfigUpdater::gotoStateUpdate() {
	parser.setFixedDecimalPoint(fixedDecimalPoint);
	parser.start();
	timer->start(1);
	offset = 0;
	stepSize = STEP_SIZE;
	if((offset + stepSize) < data->getLen()) {
		state = State_Update;
	} else {
		stepSize = data->getLen() - offset;
		state = State_UpdateLast;
	}
}

void ConfigUpdater::stateUpdateTimeout() {
	LOG_DEBUG(LOG_MODEM, "stateUpdateTimeout");
	parser.procData(data->getData() + offset, stepSize);
	timer->start(1);
	offset += stepSize;
	stepSize = STEP_SIZE;
	if((offset + stepSize) >= data->getLen()) {
		stepSize = data->getLen() - offset;
		state = State_UpdateLast;
	}
}

void ConfigUpdater::stateUpdateLastTimeout() {
	LOG_DEBUG(LOG_MODEM, "stateUpdateLastTimeout");
	parser.procData(data->getData() + offset, stepSize);
	parser.complete();
	if(parser.hasError() == true) {
		LOG_ERROR(LOG_MODEM, "Config parsing failed.");
		result = Dex::DataParser::Result_Error;
		courier.deliver(Dex::DataParser::Event_AsyncError);
		return;
	}

	result = Dex::DataParser::Result_Ok;
	state = State_Idle;
	courier.deliver(Dex::DataParser::Event_AsyncOk);
}
