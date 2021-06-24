#include "MdbSnifferEngine.h"
#include "utils/include/NetworkProtocol.h"
#include "logger/include/Logger.h"

MdbSnifferEngine::MdbSnifferEngine(StatStorage *stat) :
	state(State_Idle),
	receiver(this),
	masterReceiver(this, stat),
	currentSlave(NULL),
	currentSniffer(NULL)
{
	this->slaves = new List<MdbSlave>(3);
	this->sniffers = new List<MdbSniffer>(5);
}

MdbSnifferEngine::~MdbSnifferEngine() {
	delete sniffers;
	delete slaves;
}

void MdbSnifferEngine::init(AbstractUart *slaveUart, AbstractUart *masterUart) {
	LOG_INFO(LOG_MDBS, "init");
	this->sender.setUart(slaveUart);
	this->receiver.setUart(slaveUart);
	this->masterReceiver.setUart(masterUart);
	LOG_DEBUG(LOG_MDBS, "Mdb init complete.");
}

void MdbSnifferEngine::addSlave(MdbSlave *slave) {
	slaves->add(slave);
	slave->initSlave(&sender, &receiver);
}

void MdbSnifferEngine::addSniffer(MdbSniffer *sniffer) {
	sniffers->add(sniffer);
	sniffer->initSlave(&sender, &receiver);
}

void MdbSnifferEngine::reset() {
	LOG_ERROR(LOG_MDBS, "reset");
	this->state = State_Recv;
	this->receiver.recvAddress();
	LOG_DEBUG(LOG_MDBS, "reset complete");
}

MdbSlave *MdbSnifferEngine::getSlave(uint8_t type) {
	for(uint16_t i = 0; i < slaves->getSize(); i++) {
		MdbSlave *slave = slaves->get(i);
		if(slave->getType() == type) {
			return slave;
		}
	}
	return NULL;
}

MdbSniffer *MdbSnifferEngine::getSniffer(uint8_t type) {
	for(uint16_t i = 0; i < sniffers->getSize(); i++) {
		MdbSniffer *sniffer = sniffers->get(i);
		if(sniffer->getType() == type) {
			return sniffer;
		}
	}
	return NULL;
}

void MdbSnifferEngine::recvAddress(const uint8_t address) {
	LOG_DEBUG(LOG_MDBS, "recvAddress");
	LOG_DEBUG_HEX(LOG_MDBS, address);
	if(state != State_Recv) {
		LOG_ERROR(LOG_MDBS, "Wrong state " << state);
		return;
	}

	uint8_t device = address & MDB_DEVICE_MASK;
	currentSlave = getSlave(device);
	if(currentSlave != NULL) {
		uint8_t command = address & MDB_COMMAND_MASK;
		currentSlave->recvCommand(command);
		currentSniffer = NULL;
		return;
	} else {
		currentSniffer = getSniffer(device);
		if(currentSniffer == NULL) {
			LOG_WARN(LOG_MDBS, "Unknown slave " << device << "," << Mdb::deviceId2String(device));
			this->receiver.recvAddress();
			return;
		}
		uint8_t command = address & MDB_COMMAND_MASK;
		currentSniffer->recvCommand(command);
		return;
	}
}

void MdbSnifferEngine::recvSubcommand(const uint8_t subcommand) {
	if(currentSlave != NULL) {
		currentSlave->recvSubcommand(subcommand);
	} else if(currentSniffer != NULL) {
		currentSniffer->recvSubcommand(subcommand);
	} else {
		LOG_ERROR(LOG_MDBS, "Slave not defined");
		LOG_ERROR_HEX(LOG_MDBS, subcommand);
		return;
	}
}

void MdbSnifferEngine::recvRequest(const uint8_t *data, uint16_t len) {
	if(currentSlave != NULL) {
		currentSlave->recvRequest(data, len);
	} else if(currentSniffer != NULL) {
		currentSniffer->recvRequest(data, len);
	} else {
		LOG_ERROR(LOG_MDBS, "Slave not defined");
		LOG_ERROR_HEX(LOG_MDBS, data, len);
		return;
	}
}

void MdbSnifferEngine::recvConfirm(uint8_t control) {
	if(currentSlave != NULL) {
		currentSlave->recvConfirm(control);
	} else if(currentSniffer != NULL) {
		currentSniffer->recvConfirm(control);
	} else {
		LOG_ERROR(LOG_MDBS, "Slave not defined");
		LOG_ERROR_HEX(LOG_MDBS, control);
		return;
	}
}

void MdbSnifferEngine::procResponse(const uint8_t *data, uint16_t len, bool crc) {
	if(currentSniffer != NULL) {
		currentSniffer->procResponse(data, len, crc);
	}
}
