#include "EcpEventTable.h"

#include "config/v3/event/Config3Event.h"
#include "common/logger/include/Logger.h"

EcpEventTable::EcpEventTable(ConfigModem *config, RealTimeInterface *realtime) : config(config), realtime(realtime) {}

bool EcpEventTable::isTableExist(uint16_t tableId) {
	return (tableId == Ecp::Table_Event);
}

uint32_t EcpEventTable::getTableSize(uint16_t tableId) {
	if(tableId != Ecp::Table_Event) {
		LOG_ERROR(LOG_MODEM, "Wrong tableId " << tableId);
		return 0;
	}

	LOG_DEBUG(LOG_MODEM, "TableSize=" << config->getEvents()->getLen());
	return config->getEvents()->getLen();
}

//todo: восстановить выгрузку событий через ECP
uint16_t EcpEventTable::getTableEntry(uint16_t tableId, uint32_t entryIndex, uint8_t *buf, uint16_t bufSize) {
#if 0
	if(tableId != Ecp::Table_Event) {
		LOG_ERROR(LOG_MODEM, "Wrong tableId " << tableId);
		return 0;
	}

	uint16_t dataLen = sizeof(Config3EventStruct);
	if(dataLen > bufSize) {
		LOG_ERROR(LOG_MODEM, "Data too big " << dataLen << ">" << bufSize);
		return 0;
	}

	Config3Event event;
	if(config->getEvents()->getFromFirst(entryIndex, &event) == false) {
		LOG_ERROR(LOG_MODEM, "Load error " << entryIndex);
		DateTime datetime;
		realtime->getDateTime(&datetime);
		event.set(&datetime, ConfigEvent::Type_EventReadError);
	}

	uint8_t *data = (uint8_t*)(event.getData());
	for(uint16_t i = 0; i < dataLen; i++) {
		buf[i] = data[i];
	}

	return dataLen;
#else
	return 0;
#endif
}

uint16_t EcpEventTable::getDateTime(uint8_t *buf, uint16_t bufSize) {
	DateTime *datetime = (DateTime*)buf;
	if(sizeof(*datetime) > bufSize) {
		return 0;
	}
	realtime->getDateTime(datetime);
	return sizeof(*datetime);
}
