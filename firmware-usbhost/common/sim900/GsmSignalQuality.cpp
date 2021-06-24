#include "common/sim900/include/GsmSignalQuality.h"
#include "common/utils/include/StringParser.h"
#include "common/logger/include/Logger.h"

#include <string.h>

namespace Gsm {

SignalQuality::SignalQuality(CommandProcessor *commandProcessor, EventEngineInterface *eventEngine) :
	commandProcessor(commandProcessor),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	command(this)
{

}

bool SignalQuality::get() {
	command.set(Command::Type_CsqData, "AT+CSQ");
	return commandProcessor->execute(&command);
}

void SignalQuality::procResponse(Command::Result result, const char *data) {
	if(result != Command::Result_OK) {
		LOG_ERROR(LOG_SIM, "Command failed " << result);
		procError();
		return;
	}

	StringParser parser(data);
	uint16_t rssi;
	if(parser.getNumber(&rssi) == false) {
		procError();
		return;
	}
	if(parser.compareAndSkip(",") == false) {
		procError();
		return;
	}
	uint16_t ber;
	if(parser.getNumber(&ber) == false) {
		procError();
		return;
	}

	uint8_t qualityTable[30] = {
		 0,  4,  8, 12, 16, 20, 24, 28, 32, 36,
		40, 44, 48, 52, 56, 60, 64, 68, 72, 76,
		80, 82, 84, 86, 88, 90, 92, 94, 96, 98,
	};
	uint16_t quality = 0;
	uint16_t qualityMax = 30;
	if(rssi >= qualityMax) {
		quality = 100;
	} else {
		quality = qualityTable[rssi];
	}

	EventUint16Interface event(deviceId, Driver::Event_SignalQuality, quality);
	eventEngine->transmit(&event);
}

void SignalQuality::procError() {
	LOG_ERROR(LOG_SIM, "procError");
	EventUint16Interface event(deviceId, Driver::Event_SignalQuality, 0);
	eventEngine->transmit(&event);
}

}
