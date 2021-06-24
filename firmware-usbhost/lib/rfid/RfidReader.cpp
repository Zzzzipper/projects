#include "RfidReader.h"
#include "lib/rfid/mfrc532/RFID.h"
#include "logger/include/Logger.h"

#define RFID_MODEL "MIFARE"
#define RFID_INIT_TIMEOUT 5000
#define RFID_POLL_TIMEOUT 1500

RfidReader::RfidReader(Mdb::DeviceContext *context, TimerEngine *timerEngine, EventEngineInterface *eventEngine) :
	context(context),
	timerEngine(timerEngine),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	state(State_Idle)
{
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	timer = timerEngine->addTimer<RfidReader, &RfidReader::procTimer>(this);
	rfid = new RFID(I2C::get(I2C_1));
}

void RfidReader::reset() {
	LOG_INFO(LOG_RFID, "reset");
	context->setManufacturer((uint8_t*)MDB_MANUFACTURER_CODE, sizeof(MDB_MANUFACTURER_CODE));
	context->setModel((uint8_t*)RFID_MODEL, sizeof(RFID_MODEL));
	context->setStatus(Mdb::DeviceContext::Status_NotFound);
	gotoStateInit();
}

void RfidReader::procTimer() {
	LOG_DEBUG(LOG_RFID, "procTimer");
	switch(state) {
	case State_Init: stateInitTimeout(); break;
	case State_Poll: statePollTimeout(); break;
	default: LOG_ERROR(LOG_RFID, "Unwaited timeout state=" << state);
	}
}

void RfidReader::gotoStateInit() {
	LOG_DEBUG(LOG_RFID, "gotoStateInit");
	timer->start(RFID_POLL_TIMEOUT);
	state = State_Init;
}

void RfidReader::stateInitTimeout() {
	LOG_DEBUG(LOG_RFID, "stateInitTimeout");
	uint32_t versiondata;
	versiondata = rfid->getFirmwareVersion();
	if(versiondata == 0) {
		LOG_INFO(LOG_RFID, "Chip not found!");
		gotoStateInit();
		return;
	}

	LOG_INFO(LOG_RFID, "Found chip PN5" << ((versiondata>>24) & 0xFF));
	LOG_INFO(LOG_RFID, "Firmware ver. " << ((versiondata>>16) & 0xFF) << "." << ((versiondata>>8) & 0xFF));

	if(rfid->SAMConfig() == false) {
		LOG_ERROR(LOG_RFID, "SAMConfig failed");
		gotoStateInit();
		return;
	}
	if(rfid->prepareReadPassiveTargetID(PN532_MIFARE_ISO14443A) == false) {
		LOG_ERROR(LOG_RFID, "Prepare for read card failed");
		gotoStateInit();
		return;
	}

	context->setStatus(Mdb::DeviceContext::Status_Work);
	gotoStatePoll();
}

void RfidReader::gotoStatePoll() {
	LOG_DEBUG(LOG_RFID, "gotoStatePoll");
	timer->start(RFID_POLL_TIMEOUT);
	state = State_Poll;
}

void RfidReader::statePollTimeout() {
	LOG_DEBUG(LOG_RFID, "stateSearchTimeout");
	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
	uint8_t uidLength;
	if(rfid->preparedReadPassiveTargetID(uid, &uidLength) == false) {
		LOG_INFO(LOG_RFID, "Not found card");
		gotoStatePoll();
		return;
	}

	LOG_INFO(LOG_RFID, "Found an ISO14443A card");
	LOG_INFO(LOG_RFID, "  UID Length: " << uidLength << " bytes");
	LOG_INFO(LOG_RFID, "  UID Value: ");
	LOG_INFO_HEX(LOG_RFID, uid, uidLength);
	Rfid::EventCard event(deviceId, uid, uidLength);
	eventEngine->transmit(&event);

	if(rfid->prepareReadPassiveTargetID(PN532_MIFARE_ISO14443A) == false) {
		LOG_ERROR(LOG_RFID, "Prepare for read card failed");
		gotoStateInit();
		return;
	}

	timer->start(RFID_POLL_TIMEOUT);
}
