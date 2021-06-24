#include "include/MdbSlaveEngine.h"
#include "utils/include/NetworkProtocol.h"
#include "logger/include/Logger.h"

MdbSlaveEngine::MdbSlaveEngine() :
	state(State_Idle),
	receiver(this),
	current(NULL)
{
	this->slaves = new List<MdbSlave>(5);
}

MdbSlaveEngine::~MdbSlaveEngine() {
	delete this->slaves;
}

void MdbSlaveEngine::init(AbstractUart *uart) {
	LOG_INFO(LOG_MDBS, "Mdb init start...");
	this->uart = uart;
	this->sender.setUart(this->uart);
	this->receiver.setUart(this->uart);
	LOG_DEBUG(LOG_MDBS, "Mdb init complete.");
}

void MdbSlaveEngine::add(MdbSlave *slave) {
	slaves->add(slave);
	slave->initSlave(&sender, &receiver);
}

void MdbSlaveEngine::reset() {
	LOG_ERROR(LOG_MDBS, "reset");
	this->state = State_Recv;
	this->receiver.recvAddress();
	LOG_DEBUG(LOG_MDBS, "reset complete");
}

MdbSlave *MdbSlaveEngine::getSlave(uint8_t type) {
	for(uint16_t i = 0; i < this->slaves->getSize(); i++) {
		MdbSlave *slave = this->slaves->get(i);
		if(slave->getType() == type) {
			return slave;
		}
	}
	return NULL;
}

void MdbSlaveEngine::recvAddress(const uint8_t address) {
	LOG_DEBUG(LOG_MDBS, "recvAddress");
	LOG_DEBUG_HEX(LOG_MDBS, address);
	if(this->state != State_Recv) {
		LOG_ERROR(LOG_MDBS, "Wrong state " << this->state);
		return;
	}

	uint8_t device = address & MDB_DEVICE_MASK;
	current = this->getSlave(device);
	if(current == NULL) {
		LOG_WARN(LOG_MDBS, "Unknown slave " << device << "," << Mdb::deviceId2String(device));
		this->receiver.recvAddress();
		return;
	}

	uint8_t command = address & MDB_COMMAND_MASK;
	current->recvCommand(command);
}

void MdbSlaveEngine::recvSubcommand(const uint8_t subcommand) {
	if(current == NULL) {
		LOG_ERROR(LOG_MDBS, "Slave not defined");
		LOG_ERROR_HEX(LOG_MDBS, subcommand);
		return;
	}
	current->recvSubcommand(subcommand);
}

void MdbSlaveEngine::recvRequest(const uint8_t *data, uint16_t len) {
	if(current == NULL) {
		LOG_ERROR(LOG_MDBS, "Slave not defined");
		LOG_ERROR_HEX(LOG_MDBS, data, len);
		return;
	}
	current->recvRequest(data, len);
}

void MdbSlaveEngine::recvConfirm(uint8_t control) {
	if(current == NULL) {
		LOG_DEBUG(LOG_MDBS, "Slave not defined");
		return;
	}
	current->recvConfirm(control);
}
