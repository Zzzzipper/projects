#ifndef COMMON_GSM_SIGNAL_QUALITY_H
#define COMMON_GSM_SIGNAL_QUALITY_H

#include "sim900/command/GsmCommand.h"
#include "sim900/include/GsmDriver.h"
#include "event/include/EventEngine.h"

namespace Gsm {

class SignalQualityInterface {
public:
	virtual ~SignalQualityInterface() {}
	virtual EventDeviceId getDeviceId() = 0;
	virtual bool get() = 0;
};

class SignalQuality : public Command::Observer, public SignalQualityInterface {
public:
	SignalQuality(CommandProcessor *commandProcessor, EventEngineInterface *eventEngine);
	virtual EventDeviceId getDeviceId() { return deviceId; }
	virtual bool get();

	virtual void procResponse(Command::Result result, const char *data);
	virtual void procEvent(const char *data) { (void)data; }

private:
	CommandProcessor *commandProcessor;
	EventEngineInterface *eventEngine;
	EventDeviceId deviceId;
	Command command;

	void procError();
};

}

#endif
