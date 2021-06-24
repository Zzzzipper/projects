#include "include/MdbMasterEngine.h"

#include "logger/include/Logger.h"

enum Timeout {
	Timeout_Reset	= 150,
	Timeout_Poll	= 150,
	Timeout_Recv	= 100
};

MdbMasterEngine::MdbMasterEngine(StatStorage *stat) :
	state(State_Idle),
	receiver(this, stat)
{
	this->devices = new List<MdbMaster>(5);
	this->timeoutCount = stat->add(Mdb::DeviceContext::Info_MdbM_TimeoutCount, 0);
	this->wrongState1 = stat->add(Mdb::DeviceContext::Info_MdbM_WrongState1, 0);
	this->wrongState2 = stat->add(Mdb::DeviceContext::Info_MdbM_WrongState2, 0);
	this->wrongState3 = stat->add(Mdb::DeviceContext::Info_MdbM_WrongState3, 0);
	this->wrongState4 = stat->add(Mdb::DeviceContext::Info_MdbM_WrongState4, 0);
	this->otherError = stat->add(Mdb::DeviceContext::Info_MdbM_OtherError, 0);
}

MdbMasterEngine::~MdbMasterEngine() {
	delete this->devices;
}

MdbMaster *MdbMasterEngine::getDevice(Mdb::Device deviceId) {
	for(uint16_t i = 0; i < this->devices->getSize(); i++) {
		MdbMaster *device = this->devices->get(i);
		if(device->getType() == deviceId) {
			return device;
		}
	}
	return NULL;	
}

void MdbMasterEngine::init(AbstractUart *uart, TimerEngine *timers) {
	LOG_INFO(LOG_MDBM, "init");
	this->uart = uart;
	
	this->sender.setUart(this->uart);
	this->receiver.setUart(this->uart);

	this->timerReset = timers->addTimer<MdbMasterEngine, &MdbMasterEngine::procResetTimer>(this, TimerEngine::ProcInTick);
	this->timerPoll = timers->addTimer<MdbMasterEngine, &MdbMasterEngine::procPollTimer>(this, TimerEngine::ProcInTick);
	this->timerRecv = timers->addTimer<MdbMasterEngine, &MdbMasterEngine::procRecvTimer>(this, TimerEngine::ProcInTick);

	LOG_DEBUG(LOG_MDBM, "Mdb init complete.");
}

void MdbMasterEngine::add(MdbMaster *device) {
	if(device == NULL) { return; }
	devices->add(device);
	device->initMaster(&sender);
}

void MdbMasterEngine::reset() {
	LOG_ERROR(LOG_MDBM, "reset");
	this->timerReset->start(Timeout_Reset);
	this->timerPoll->stop();
	this->timerRecv->stop();
	this->state = State_Reset;
	LOG_DEBUG(LOG_MDBM, "reset complete");
}

void MdbMasterEngine::procResetTimer() {
	LOG_DEBUG(LOG_MDBM, "procResetTimer");
	if(this->state != State_Reset) {
		LOG_ERROR(LOG_MDBM, "Wrong state (expected " << State_Reset << ", actual " << this->state << ").");
		wrongState1->inc();
		return;
	}

	this->timerPoll->start(Timeout_Poll);
	this->current = 0;
	this->pollSlave();
}

void MdbMasterEngine::procPollTimer() {
	LOG_DEBUG(LOG_MDBM, "procPollTimer");
	if(this->state == State_Recv) {
		this->timerPoll->start(Timeout_Recv);
		return;
	}
	if(this->state != State_Poll) {
		LOG_ERROR(LOG_MDBM, "Wrong state (expected " << State_Poll << ", actual " << this->state << ").");
		wrongState2->inc();
		return;
	}

	this->timerPoll->start(Timeout_Poll);
	this->current = 0;
	this->pollSlave();
}

void MdbMasterEngine::pollSlave() {
	LOG_DEBUG(LOG_MDBM, "pollSlave");
	if(current >= devices->getSize()) {
		state = State_Poll;
		return;
	}

	MdbMaster *device = devices->get(current);
	timerRecv->start(Timeout_Recv);
	device->sendRequest();
	state = State_Recv;
}

void MdbMasterEngine::procRecvTimer() {
	LOG_DEBUG(LOG_MDBM, "procRecvTimer");
	if(state != State_Recv) {
		LOG_ERROR(LOG_MDBM, "Wrong state (expected " << State_Recv << ", actual " << state << ").");
		wrongState3->inc();
		return;
	}
	if(current >= devices->getSize()) {
		LOG_ERROR(LOG_MDBM, "Slave index too big.");
		otherError->inc();
		return;
	}

	MdbMaster *device = devices->get(current);
	device->timeoutResponse();
	timeoutCount->inc();

	current++;
	pollSlave();
}

void MdbMasterEngine::procResponse(const uint8_t *data, uint16_t dataLen, bool crc) {
	LOG_DEBUG(LOG_MDBM, "procResponse");
	if(state != State_Recv) {
		LOG_ERROR(LOG_MDBM, "Wrong state (expected " << State_Recv << ", actual " << state << ", crc " << crc << ").");
		LOG_ERROR_HEX(LOG_MDBM, data, dataLen);
		wrongState4->inc();
		return;
	}
	if(current >= devices->getSize()) {
		LOG_ERROR(LOG_MDBM, "Slave index too big.");
		otherError->inc();
		return;
	}

	timerRecv->stop();
	MdbMaster *device = devices->get(current);
	device->recvResponse(data, dataLen, crc);

	current++;
	pollSlave();
}
