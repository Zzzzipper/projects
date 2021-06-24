#include "EvadtsEventProcessor.h"

#include "common/logger/include/Logger.h"

#include <strings.h>

void EvadtsEventProcessor::proc(
	const char *name,
	uint8_t activity,
	uint8_t duration,
	DateTime *datetime,
	ConfigModem *config
) {
	ConfigEvent::Code code = nameToCode(name, activity);
	if(code == ConfigEvent::Type_DtsUnwaitedEvent) {
		String eventInfo;
		eventInfo << name << "*" << activity << "*" << duration << "*";
		datetime2string(datetime, &eventInfo);
		config->getEvents()->add(ConfigEvent::Type_DtsUnwaitedEvent, eventInfo.getString());
		return;
	}
	if(isError(name) == true) {
		if(activity > 0) {
			LOG_DEBUG(LOG_SM, "Add event " << code);
			if(config->getAutomat()->getSMContext()->addError(code, "") == true) {
				config->getEvents()->add(code, "");
			}
			return;
		} else {
			LOG_DEBUG(LOG_SM, "Remove event " << code);
			config->getAutomat()->getSMContext()->removeError(code);
			return;
		}
	} else {
		LOG_DEBUG(LOG_SM, "Add event " << code);
		config->getEvents()->add(code, "");
		return;
	}
}

bool EvadtsEventProcessor::isError(const char *name) {
	if(strncasecmp(name, "EGS", 3) == 0) { return false; }
	if(strncasecmp(name, "EBJ", 3) == 0) { return true; }
	if(strncasecmp(name, "EFL", 3) == 0) { return true; }
	if(strncasecmp(name, "OBI", 3) == 0) { return true; }
	if(strncasecmp(name, "EEA", 3) == 0) { return true; }
	if(strncasecmp(name, "EDT", 3) == 0) { return true; } // Fault on grinder
	if(strncasecmp(name, "ED", 2) == 0) { return true; } // General non-specific Hot Drinks System fault
	else { return false; }
}

ConfigEvent::Code EvadtsEventProcessor::nameToCode(const char *name, uint8_t activity) {
	if(strncasecmp(name, "EGS", 3) == 0) { return activity == 1 ? ConfigEvent::Type_EGS_DoorOpen : ConfigEvent::Type_EGS_DoorClose; }
	if(strncasecmp(name, "EBJ", 3) == 0) { return ConfigEvent::Type_EBJ_NoCups; }
	if(strncasecmp(name, "EFL", 3) == 0) { return ConfigEvent::Type_EFL_NoWater; }
	if(strncasecmp(name, "OBI", 3) == 0) { return ConfigEvent::Type_OBI_WasteWaterPipe; }
	if(strncasecmp(name, "EEA", 3) == 0) { return ConfigEvent::Type_EEA_CoffeeMotorFault; }
	if(strncasecmp(name, "EDT", 3) == 0) { return ConfigEvent::Type_EDT_Grinder; }
	if(strncasecmp(name, "ED", 2) == 0) { return ConfigEvent::Type_ED_HotDrinkFault; }
	else { return ConfigEvent::Type_DtsUnwaitedEvent; }
}
