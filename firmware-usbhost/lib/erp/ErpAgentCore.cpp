#include "ErpAgentCore.h"

#include "lib/battery/PowerAgent.h"

#include "common/sim900/include/GsmSignalQuality.h"
#include "common/http/include/HttpClient.h"
#include "common/utils/include/Utils.h"
#include "common/utils/include/Version.h"
#include <logger/RemoteLogger.h>
#include "common/logger/include/Logger.h"

#ifdef PING_SERVER
#define ERP_SYNC_INIT_TIMEOUT 60*1000
#define ERP_SYNC_NEXT_TIMEOUT 30*1000
#define ERP_SYNC_TIMEOUT 120*1000 // уменьшен в 2,5 раза
#define ERP_SYNC_COUNT 12
#define ERP_SYNC_ERROR 60
#define REBOOT_TIMEOUT 1
#define CLOCK_DEVIATION 30
#else
#define ERP_SYNC_INIT_TIMEOUT 60*1000
#define ERP_SYNC_NEXT_TIMEOUT 30*1000
#define ERP_SYNC_TIMEOUT 300*1000
#define ERP_SYNC_COUNT 12
#define ERP_SYNC_ERROR 12
#define REBOOT_TIMEOUT 1
#define CLOCK_DEVIATION 30
#endif

ErpAgentCore::ErpAgentCore(
	ConfigMaster *config,
	TimerEngine *timers,
	EventEngineInterface *eventEngine,
	Network *network,
	Gsm::SignalQualityInterface *signalQuality,
	ErpProtocolInterface *erp,
	EcpInterface *ecp,
	ErpCashless *cashless,
	VerificationInterface *verification,
	GramophoneInterface *gramophone,
	RealTimeInterface *realtime,
	RelayInterface *relay,
	PowerInterface *power,
	LedInterface *leds
) :
	config(config),
	timers(timers),
	eventEngine(eventEngine),
	network(network),
	signalQuality(signalQuality),
	erp(erp),
	ecp(ecp),
	cashless(cashless),
	verification(verification),
	gramophone(gramophone),
	realtime(realtime),
	relay(relay),
	power(power),
	leds(leds),
	stat(config->getConfig()->getStat()),
	deviceId(eventEngine),
	generator(config->getConfig()),
	poweroff(false)
{
	this->state = stat->add(Mdb::DeviceContext::Info_Erp_State, State_Idle);
	this->syncErrorCount = stat->add(Mdb::DeviceContext::Info_Erp_SyncErrorCount, 0);
	this->syncErrorMax = stat->add(Mdb::DeviceContext::Info_Erp_SyncErrorMax, 0);
	this->timer = timers->addTimer<ErpAgentCore, &ErpAgentCore::procTimer>(this);
	this->syncTimer = timers->addTimer<ErpAgentCore, &ErpAgentCore::procSyncTimer>(this);
	this->melodyAudit = new MelodyElochkaHalf;
	this->melodyConfig = new MelodyImpireMarch;
	this->updater = new ConfigUpdater(timers, config->getConfig(), this);
	this->erp->setObserver(this);
	this->eventEngine->subscribe(this, GlobalId_Sim900);
}

ErpAgentCore::~ErpAgentCore() {
	delete updater;
	delete melodyConfig;
	delete melodyAudit;
	timers->deleteTimer(syncTimer);
	timers->deleteTimer(timer);
}

void ErpAgentCore::reset() {
	leds->setServer(LedInterface::State_Off);
	command = CommandType_None;
	syncCount = ERP_SYNC_COUNT;
	syncError = 0;
	syncTimer->start(ERP_SYNC_INIT_TIMEOUT);
	gotoStateWait();
}

void ErpAgentCore::syncEvents() {
	LOG_INFO(LOG_MODEM, "syncEvents");
	poweroff = false;
	if(state->get() != State_Wait) {
		LOG_INFO(LOG_MODEM, "Wrong state " << state->get());
		command = CommandType_Sync;
		return;
	}
	gotoStateSignalQuality();
}

void ErpAgentCore::ping() {
	LOG_INFO(LOG_MODEM, "ping");
	poweroff = false;
	if(state->get() != State_Wait) {
		LOG_INFO(LOG_MODEM, "Wrong state " << state->get());
		command = CommandType_Sync;
		return;
	}
	gotoStatePing();
}

void ErpAgentCore::powerDown() {
	LOG_INFO(LOG_MODEM, "powerDown");
	poweroff = true;
	if(state->get() != State_Wait) {
		LOG_INFO(LOG_MODEM, "Wrong state " << state->get());
		command = CommandType_Sync;
		return;
	}
	gotoStateSignalQuality();
}

void ErpAgentCore::sendAudit() {
	LOG_INFO(LOG_MODEM, "sendAudit");
	if(state->get() != State_Wait) {
		LOG_INFO(LOG_MODEM, "Wrong state " << state->get());
		command = CommandType_Audit;
		return;
	}
	gotoStateAuditLoad(true);
}

void ErpAgentCore::loadConfig() {
	LOG_INFO(LOG_MODEM, "loadConfig");
	if(state->get() != State_Wait) {
		LOG_INFO(LOG_MODEM, "Wrong state " << state->get());
		command = CommandType_Audit;
		return;
	}
	gotoStateConfigLoad();
}

void ErpAgentCore::sendDebug() {
	LOG_INFO(LOG_MODEM, "sendDebug");
	if(state->get() != State_Wait) {
		LOG_INFO(LOG_MODEM, "Wrong state " << state->get());
		command = CommandType_Debug;
		return;
	}
	gotoSendDebug();
}

void ErpAgentCore::procTimer() {
	LOG_DEBUG(LOG_MODEM, "procTimer");
	switch(state->get()) {
	case State_AuditGenerate: stateAuditGenerateTimeout(); break;
	default: LOG_DEBUG(LOG_MODEM, "Wrong state " << state->get());
	}
}

void ErpAgentCore::procSyncTimer() {
	LOG_DEBUG(LOG_MODEM, "procSyncTimer" << state->get());
	syncTimer->start(ERP_SYNC_TIMEOUT);
	switch(state->get()) {
	case State_Wait: stateWaitTimeout(); break;
	default: procSyncUnwaitedTimeout(); break;
	}
}

void ErpAgentCore::proc(Event *event) {
	switch(state->get()) {
		case State_Sync: stateSyncEvent(event); break;
		case State_Ping: statePingEvent(event); break;
		case State_AuditLoad: stateAuditLoadEvent(event); break;
		case State_AuditSend: stateAuditSendEvent(event); break;
		case State_AuditMelody: stateAuditMelodyEvent(event); break;
		case State_ConfigLoad: stateConfigLoadEvent(event); break;
		case State_ConfigUpdate: stateConfigUpdateEvent(event); break;
		case State_ConfigMelody: stateConfigMelodyEvent(event); break;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << state->get() << "," << event->getType());
	}
}

void ErpAgentCore::proc(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_SM, "proc " << state->get() << "," << envelope->getType());
	switch(state->get()) {
		case State_SignalQuality: stateSignalQualityEvent(envelope); break;
		default: LOG_ERROR(LOG_SM, "Unwaited event " << state->get() << "," << envelope->getType());
	}
}

bool ErpAgentCore::procRemoteCommand(uint32_t remoteCommand) {
	LOG_INFO(LOG_MODEM, "procRemoteCommand " << remoteCommand);
	if(remoteCommand == ErpProtocol::Command_ReloadModem) {
		LOG_INFO(LOG_MODEM, "Reload modem");
		gotoStateReboot();
		return true;
	}
	if(remoteCommand == ErpProtocol::Command_LoadConfig) {
		LOG_INFO(LOG_MODEM, "Load config");
		gotoStateConfigLoad();
		return true;
	}
	if(remoteCommand == ErpProtocol::Command_LoadAudit) {
		LOG_INFO(LOG_MODEM, "Load audit");
		gotoStateAuditLoad(true);
		return true;
	}
	if(remoteCommand == ErpProtocol::Command_LoadStat) {
		LOG_INFO(LOG_MODEM, "Load stat");
		gotoStateAuditLoad(false);
		return true;
	}
	if(remoteCommand == ErpProtocol::Command_ReloadAutomat) {
		LOG_INFO(LOG_MODEM, "Reload automat");
		if(relay != NULL) {	relay->on(); }
	}
	if(remoteCommand == ErpProtocol::Command_ResetErrors) {
		LOG_INFO(LOG_MODEM, "Reset errors");
		ConfigAutomat *configAutomat = config->getConfig()->getAutomat();
		configAutomat->getSMContext()->getErrors()->removeAll();
		configAutomat->getCCContext()->getErrors()->removeAll();
		configAutomat->getBVContext()->getErrors()->removeAll();
		configAutomat->getMdb1CashlessContext()->getErrors()->removeAll();
		configAutomat->getMdb2CashlessContext()->getErrors()->removeAll();
		configAutomat->getExt1CashlessContext()->getErrors()->removeAll();
		configAutomat->getFiscalContext()->getErrors()->removeAll();
	}
	if(remoteCommand == ErpProtocol::Command_ResetAuditCounts) {
		LOG_INFO(LOG_MODEM, "Reset audit counts");
		config->getConfig()->getAutomat()->getProductList()->resetTotal();
	}
	if(remoteCommand == ErpProtocol::Command_SverkaItogov) {
		LOG_INFO(LOG_MODEM, "Sverka itogov");
		if(verification != NULL) { verification->verification(); }
	}
	if(remoteCommand == ErpProtocol::Command_ChargeCash) {
		LOG_INFO(LOG_MODEM, "Charge cash");
		ConfigAutomat *configAutomat = config->getConfig()->getAutomat();
		cashless->deposite(configAutomat->getMaxCredit());
	}
	if(remoteCommand == ErpProtocol::Command_UpdateFirmware) {
		LOG_INFO(LOG_MODEM, "Updare firmware");
		ConfigBoot *configBoot = config->getConfig()->getBoot();
		configBoot->setFirmwareState(ConfigBoot::FirmwareState_UpdateRequired);
		configBoot->save();
		gotoStateReboot();
		return true;
	}
	return false;
}

void ErpAgentCore::procLockError() {
	LOG_ERROR(LOG_MODEM, "procLockError");
	syncError++;
	leds->setServer(LedInterface::State_Failure);
	gotoStateWait();
}

void ErpAgentCore::procServerError() {
	LOG_ERROR(LOG_MODEM, "procServerError");
	syncError++;
	if(poweroff == true) {
		LOG_INFO(LOG_MODEM, "Sync stopped");
		power->off();
		return;
	}
	config->unlock();
	leds->setServer(LedInterface::State_Failure);
	gotoStateWait();
}

void ErpAgentCore::procSyncUnwaitedTimeout() {
	LOG_ERROR(LOG_MODEM, "procSyncUnwaitedTimeout " << state->get());
	syncError++;
	if(syncError >= ERP_SYNC_ERROR) {
		LOG_ERROR(LOG_MODEM, "Restart sync engine");
		command = CommandType_None;
		syncCount = ERP_SYNC_COUNT;
		syncError = 0;
		network->restart();
		config->unlock();
		state->set(State_Wait);
	}
}

void ErpAgentCore::gotoStateWait() {
	LOG_DEBUG(LOG_MODEM, "gotoStateWait " << command);
	if(command == CommandType_Sync) {
		command = CommandType_None;
		gotoStateSignalQuality();
		return;
	} else if(command == CommandType_Audit) {
		command = CommandType_None;
		gotoStateAuditLoad(true);
		return;
	} else if(command == CommandType_Debug) {
		command = CommandType_None;
		gotoSendDebug();
		return;
	}

	LOG_DEBUG(LOG_MODEM, "SyncTimerStart");
	state->set(State_Wait);
}

void ErpAgentCore::stateWaitTimeout() {
	LOG_DEBUG(LOG_MODEM, "stateWaitTimeout");
	syncCount++;

	ConfigEventList *events = config->getConfig()->getEvents();
	if(events->getUnsync() == CONFIG_EVENT_UNSET && syncCount < ERP_SYNC_COUNT) {
		LOG_ERROR(LOG_MODEM, "Nothing to sync");
#ifdef PING_SERVER
		gotoStatePing();
#else
		gotoStateWait();
#endif
		return;
	}

	gotoStateSignalQuality();
}

void ErpAgentCore::gotoStateSignalQuality() {
	LOG_DEBUG(LOG_MODEM, "gotoStateSingalQuality");
	if(signalQuality == NULL) {
		signalValue = 0;
		gotoStateSync();
		return;
	}

	if(signalQuality->get() == false) {
		LOG_ERROR(LOG_MODEM, "Command failed");
		procServerError();
		return;
	}

	state->set(State_SignalQuality);
}

void ErpAgentCore::stateSignalQualityEvent(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_MODEM, "stateSignalQualityEvent");
	switch(envelope->getType()) {
		case Gsm::Driver::Event_SignalQuality: stateSignalQualityEventComplete(envelope); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << envelope->getType() << "," << state->get());
	}
}

void ErpAgentCore::stateSignalQualityEventComplete(EventEnvelope *envelope) {
	LOG_DEBUG(LOG_MODEM, "stateSignalQualityEventComplete");
	EventUint16Interface event(Gsm::Driver::Event_SignalQuality);
	if(event.open(envelope) == false) {
		LOG_ERROR(LOG_MODEM, "Wrong envelope");
		procServerError();
		return;
	}

	signalValue = event.getValue();
	gotoStateSync();
}

void ErpAgentCore::gotoStateSync() {
	LOG_DEBUG(LOG_MODEM, "gotoStateSync");
	if(config->lock() == false) {
		LOG_ERROR(LOG_MODEM, "Config locked");
		procLockError();
		return;
	}

	ConfigBoot *boot = config->getConfig()->getBoot();
	ConfigAutomat *automat = config->getConfig()->getAutomat();
	ConfigEventList *events = config->getConfig()->getEvents();
	if(erp->sendSyncRequest(boot->getImei(), automat->getDecimalPoint(), signalValue, events, config->getBuffer()) == false) {
		LOG_ERROR(LOG_MODEM, "Send event error");
		procServerError();
		return;
	}

	leds->setServer(LedInterface::State_InProgress);
	state->set(State_Sync);
}

void ErpAgentCore::stateSyncEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateSyncEvent");
	switch(event->getType()) {
		case Http::Client::Event_RequestComplete: stateSyncEventComplete(event); return;
		case Http::Client::Event_RequestError: stateSyncEventError(); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state->get());
	}
}

void ErpAgentCore::stateSyncEventComplete(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateSyncEventComplete");
	syncError = 0;
	config->unlock();
	leds->setServer(LedInterface::State_Success);

	if(poweroff == true) {
		LOG_INFO(LOG_MODEM, "Sync stopped");
		power->off();
		return;
	}

	ErpProtocol::EventSync *eventSync = (ErpProtocol::EventSync *)event;
	updateDateTime(&eventSync->dt);
	if(updateFirmware(eventSync->update) == true) {
		LOG_INFO(LOG_MODEM, "Update firmware");
		gotoStateReboot();
		return;
	}
	if(procRemoteCommand(eventSync->command) == true) {
		LOG_INFO(LOG_MODEM, "Stop sync");
		return;
	}

	syncCount = 0;
	if(config->getConfig()->getEvents()->getUnsync() == CONFIG_EVENT_UNSET) {
		gotoStateWait();
	} else {
		gotoStateSync();
	}
}

bool ErpAgentCore::updateFirmware(uint8_t update) {
	ConfigBoot *boot = config->getConfig()->getBoot();
	LOG_DEBUG(LOG_MODEM, "updateFirmware " << update);
	if(update == ConfigBoot::FirmwareRelease_None) {
		return false;
	}

	LOG_ERROR(LOG_MODEM, "Start updating to release " << update);
	boot->setFirmwareState(ConfigBoot::FirmwareState_UpdateRequired);
	boot->setFirmwareRelease(update);
	boot->save();
	return true;
}

void ErpAgentCore::updateDateTime(DateTime *serverDatetime) {
	LOG_DEBUG(LOG_MODEM, "updateDateTime");
	DateTime localDatetime;
	realtime->getDateTime(&localDatetime);
	int32_t dif = localDatetime.secondsTo(serverDatetime);
	if(dif < -CLOCK_DEVIATION || dif > CLOCK_DEVIATION) {
		LOG_ERROR(LOG_MODEM, "Set datetime from " << LOG_DATETIME(localDatetime) << " to " << LOG_DATETIME(*serverDatetime));
		realtime->setDateTime(serverDatetime);
	}
}

void ErpAgentCore::stateSyncEventError() {
	LOG_DEBUG(LOG_MODEM, "stateSyncEventError");
	syncErrorCount->inc();
	procServerError();
}

void ErpAgentCore::gotoStatePing() {
	LOG_DEBUG(LOG_MODEM, "gotoStatePing");
	if(config->lock() == false) {
		LOG_ERROR(LOG_MODEM, "Config locked");
		procLockError();
		return;
	}

	ConfigBoot *boot = config->getConfig()->getBoot();
	ConfigAutomat *automat = config->getConfig()->getAutomat();
	if(erp->sendPingRequest(boot->getImei(), automat->getDecimalPoint(), config->getBuffer()) == false) {
		LOG_ERROR(LOG_MODEM, "Send event error");
		procServerError();
		return;
	}

	leds->setServer(LedInterface::State_InProgress);
	state->set(State_Ping);
}

void ErpAgentCore::statePingEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "statePingEvent");
	switch(event->getType()) {
		case Http::Client::Event_RequestComplete: statePingEventComplete(event); return;
		case Http::Client::Event_RequestError: statePingEventError(); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state->get());
	}
}

void ErpAgentCore::statePingEventComplete(Event *event) {
	LOG_DEBUG(LOG_MODEM, "statePingEventComplete");
	syncError = 0;
	config->unlock();
	leds->setServer(LedInterface::State_Success);

	if(poweroff == true) {
		LOG_INFO(LOG_MODEM, "Sync stopped");
		power->off();
		return;
	}

	ErpProtocol::EventSync *eventSync = (ErpProtocol::EventSync *)event;
	updateDateTime(&eventSync->dt);
	if(procRemoteCommand(eventSync->command) == true) {
		LOG_INFO(LOG_MODEM, "Stop sync");
		return;
	}

	if(config->getConfig()->getEvents()->getUnsync() == CONFIG_EVENT_UNSET) {
		gotoStateWait();
	} else {
		gotoStateSync();
	}
}

void ErpAgentCore::statePingEventError() {
	LOG_DEBUG(LOG_MODEM, "statePingEventError");
	syncErrorCount->inc();
	procServerError();
}

void ErpAgentCore::gotoStateAuditLoad(bool auditType) {
	LOG_INFO(LOG_MODEM, "gotoStateAuditLoad");
	this->auditType = auditType;

	if(ecp == NULL || config->getConfig()->getAutomat()->getEvadts() == false) {
		gotoStateAuditGenerate();
		return;
	}

	if(ecp->loadAudit(this) == false) {
		LOG_ERROR(LOG_MODEM, "Load failed");
		procLockError();
		return;
	}
	leds->setServer(LedInterface::State_InProgress);
	state->set(State_AuditLoad);
}

void ErpAgentCore::stateAuditLoadEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateAuditLoadEvent");
	switch(event->getType()) {
		case Dex::DataParser::Event_AsyncOk: stateAuditLoadEventCompete(); return;
		case Dex::DataParser::Event_AsyncError: stateAuditLoadEventError(); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << state->get() << "," << event->getType());
	}
}

void ErpAgentCore::stateAuditLoadEventCompete() {
	LOG_DEBUG(LOG_MODEM, "stateAuditLoadEventCompete");
	gotoStateAuditSend();
}

void ErpAgentCore::stateAuditLoadEventError() {
	LOG_DEBUG(LOG_MODEM, "stateAuditLoadEventError");
	procLockError();
}

void ErpAgentCore::gotoStateAuditGenerate() {
	LOG_DEBUG(LOG_MODEM, "gotoStateAuditGenerate");
	if(config->lock() == false) {
		LOG_ERROR(LOG_MODEM, "Config locked");
		procLockError();
		return;
	}

	generator.reset();
	config->getBuffer()->clear();
	config->getBuffer()->addStr((const char*)generator.getData(), generator.getLen());
	timer->start(1);
	leds->setServer(LedInterface::State_InProgress);
	state->set(State_AuditGenerate);
}

void ErpAgentCore::stateAuditGenerateTimeout() {
	LOG_DEBUG(LOG_MODEM, "stateAuditGenerateTimeout");
	if(generator.isLast() == false) {
		generator.next();
		config->getBuffer()->addStr((const char*)generator.getData(), generator.getLen());
		timer->start(1);
		return;
	}

	gotoStateAuditSend();
}

void ErpAgentCore::gotoStateAuditSend() {
	LOG_DEBUG(LOG_MODEM, "gotoStateAuditSend");
	if(erp->sendAuditRequest(config->getConfig()->getBoot()->getImei(), auditType, config->getBuffer()) == false) {
		LOG_ERROR(LOG_MODEM, "Send audit failed");
		procServerError();
		return;
	}

	LOG_STR(config->getBuffer()->getString(), config->getBuffer()->getLen());
	leds->setServer(LedInterface::State_InProgress);
	state->set(State_AuditSend);
}

void ErpAgentCore::stateAuditSendEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateAuditSendEvent");
	switch(event->getType()) {
		case Http::Client::Event_RequestComplete: stateAuditSendEventComplete(event->getUint32()); return;
		case Http::Client::Event_RequestError: procServerError(); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state->get());
	}
}

void ErpAgentCore::stateAuditSendEventComplete(uint32_t configId) {
	LOG_DEBUG(LOG_MODEM, "stateAuditSendEventComplete");
	config->getConfig()->getAutomat()->reset();

	if(config->getConfig()->getAutomat()->getConfigId() != configId) {
		LOG_DEBUG(LOG_MODEM, "Detected newest config.");
		this->configId = configId;
		gotoStateConfigLoad();
		return;
	}

	LOG_DEBUG(LOG_MODEM, "Config already synced.");
	config->unlock();
	leds->setServer(LedInterface::State_Success);
	gotoStateAuditMelody();
}

void ErpAgentCore::gotoStateAuditMelody() {
	LOG_DEBUG(LOG_MODEM, "gotoStateAuditMelody");
	gramophone->play(melodyAudit, this);
	state->set(State_AuditMelody);
}

void ErpAgentCore::stateAuditMelodyEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateAuditMelodyEvent");
	switch(event->getType()) {
		case GramophoneInterface::Event_Complete: gotoStateWait(); return;
	}
}

void ErpAgentCore::gotoSendDebug() {
	LOG_DEBUG(LOG_MODEM, "gotoSendDebug");
	if(config->lock() == false) {
		LOG_ERROR(LOG_MODEM, "Config locked");
		procLockError();
		return;
	}

	config->getBuffer()->clear();
	config->getBuffer()->set(RemoteLogger::get()->getData(), RemoteLogger::get()->getDataLen());
	leds->setServer(LedInterface::State_InProgress);
	gotoStateAuditSend();
}

void ErpAgentCore::gotoStateConfigLoad() {
	LOG_DEBUG(LOG_MODEM, "gotoStateConfigLoad");
	if(erp->sendConfigRequest(config->getConfig()->getBoot()->getImei(), config->getBuffer()) == false) {
		LOG_ERROR(LOG_MODEM, "Send audit failed");
		procServerError();
		return;
	}

	leds->setServer(LedInterface::State_InProgress);
	state->set(State_ConfigLoad);
}

void ErpAgentCore::stateConfigLoadEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateConfigLoadEvent");
	switch(event->getType()) {
		case Http::Client::Event_RequestComplete: stateConfigLoadEventComplete(); return;
		case Http::Client::Event_RequestError: stateConfigLoadEventError(); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state->get());
	}
}

void ErpAgentCore::stateConfigLoadEventComplete() {
	LOG_INFO(LOG_MODEM, "stateConfigLoadEventComplete");
	LOG_DEBUG_STR(LOG_MODEM, (char*)config->getBuffer()->getData(), config->getBuffer()->getLen());
	if(updater->checkCrc(config->getBuffer()) == false) {
		LOG_ERROR(LOG_MODEM, "Wrong config crc");
		config->getConfig()->getEvents()->add(ConfigEvent::Type_ConfigLoadFailed);
		procServerError();
		return;
	}

	gotoStateConfigUpdate();
}

void ErpAgentCore::stateConfigLoadEventError() {
	LOG_INFO(LOG_MODEM, "stateConfigLoadEventError");
	config->getConfig()->getEvents()->add(ConfigEvent::Type_ConfigLoadFailed);
	procServerError();
}

void ErpAgentCore::gotoStateConfigUpdate() {
	LOG_INFO(LOG_MODEM, "gotoStateConfigUpdate");
	updater->resize(config->getBuffer(), false);
	state->set(State_ConfigUpdate);
}

void ErpAgentCore::stateConfigUpdateEvent(Event *event) {
	LOG_INFO(LOG_MODEM, "stateConfigUpdateEvent");
	LOG_DEBUG(LOG_MODEM, "stateConfigLoadEvent");
	switch(event->getType()) {
		case Dex::DataParser::Event_AsyncOk: stateConfigUpdateEventComplete(); return;
		case Dex::DataParser::Event_AsyncError: stateConfigUpdateEventError(event); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state->get());
	}
}

void ErpAgentCore::stateConfigUpdateEventComplete() {
	LOG_INFO(LOG_MODEM, "stateConfigUpdateEventComplete");
	leds->setServer(LedInterface::State_Success);
	ConfigAutomat *automat = config->getConfig()->getAutomat();
	automat->setConfigId(this->configId);
	automat->save();
	config->getConfig()->getEvents()->add(ConfigEvent::Type_ConfigLoaded);
	config->unlock();
	gotoStateConfigMelody();
}

void ErpAgentCore::stateConfigUpdateEventError(Event *event) {
	(void)event;
	LOG_INFO(LOG_MODEM, "stateConfigUpdateEventError");
	config->getConfig()->getEvents()->add(ConfigEvent::Type_ConfigParseFailed);
	procServerError();
}

void ErpAgentCore::gotoStateConfigMelody() {
	LOG_DEBUG(LOG_MODEM, "gotoStateConfigMelody");
	gramophone->play(melodyConfig, this);
	state->set(State_ConfigMelody);
}

void ErpAgentCore::stateConfigMelodyEvent(Event *event) {
	LOG_DEBUG(LOG_MODEM, "stateConfigMelodyEvent");
	switch(event->getType()) {
		case GramophoneInterface::Event_Complete: gotoStateReboot(); return;
		default: LOG_ERROR(LOG_MODEM, "Unwaited event " << event->getType() << "," << state->get());
	}
}

void ErpAgentCore::gotoStateReboot() {
	LOG_INFO(LOG_MODEM, "gotoStateReboot");
	state->set(State_Reboot);
	EventInterface event(deviceId, SystemEvent_Reboot);
	eventEngine->transmit(&event);
}

