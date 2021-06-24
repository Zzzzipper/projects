#if 1

#include "MdbBridge.h"
#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"

MdbBridge::SlaveReceiveHandler::SlaveReceiveHandler(AbstractUart *recvUart, AbstractUart *sendUart) :
	recvUart(recvUart),
	sendUart(sendUart),
	enabled(false)
{
	recvUart->setReceiveHandler(this);
}

void MdbBridge::SlaveReceiveHandler::handle() {
	while(recvUart->isEmptyReceiveBuffer() == false) {
		uint8_t b1 = recvUart->receive();
		uint8_t b2 = recvUart->receive();
		if(enabled == true) {
			if(b1 > 0) {
				*Logger::get() << Logger::endl; *Logger::get() << "s"; Logger::get()->hex(b2);
			} else {
				*Logger::get() << "s"; Logger::get()->hex(b2);
			}
		}
		sendUart->send(b1);
		sendUart->send(b2);
	}
}

MdbBridge::MasterReceiveHandler::MasterReceiveHandler(AbstractUart *recvUart, AbstractUart *sendUart, uint8_t deviceType, SlaveReceiveHandler *slaveHandler) :
	recvUart(recvUart),
	sendUart(sendUart),
	deviceType(deviceType),
	slaveHandler(slaveHandler),
	enabled(true)
{
	recvUart->setReceiveHandler(this);
}

void MdbBridge::MasterReceiveHandler::handle() {
	while(recvUart->isEmptyReceiveBuffer() == false) {
		uint8_t b1 = recvUart->receive();
		uint8_t b2 = recvUart->receive();
		if(b1 > 0) {
			if(deviceType == 0 || ((b2 & MDB_DEVICE_MASK) == deviceType)) {
				*Logger::get() << Logger::endl; *Logger::get() << Logger::endl; *Logger::get() << "m"; Logger::get()->hex(b2);
				enabled = true;
				slaveHandler->enable(enabled);
			} else {
				enabled = false;
				slaveHandler->enable(enabled);
			}
		} else {
			if(enabled == true) {
				*Logger::get() << "m"; Logger::get()->hex(b2);
			}
		}
		sendUart->send(b1);
		sendUart->send(b2);
	}
}

MdbBridge::MdbBridge(AbstractUart *masterUart, AbstractUart *slaveUart) {
	slaveHandler = new SlaveReceiveHandler(masterUart, slaveUart);
//	masterHandler = new MasterReceiveHandler(slaveUart, masterUart, Mdb::Device_ComGateway, slaveHandler);
	masterHandler = new MasterReceiveHandler(slaveUart, masterUart, 0, slaveHandler);
}

#elif 0
#include "MdbBridge.h"

#include "logger/include/Logger.h"

MdbBridge::ReceiveHandler::ReceiveHandler(bool master, AbstractUart *recvUart, AbstractUart *sendUart) :
	master(master),
	recvUart(recvUart),
	sendUart(sendUart)
{
	recvUart->setReceiveHandler(this);
}

void MdbBridge::ReceiveHandler::handle() {
	while(recvUart->isEmptyReceiveBuffer() == false) {
		uint8_t b1 = recvUart->receive();
		uint8_t b2 = recvUart->receive();
		if(master == true) {
			if(b1 > 0) {
				*Logger::get() << Logger::endl; *Logger::get() << "m"; Logger::get()->hex(b2);
			} else {
				*Logger::get() << "m"; Logger::get()->hex(b2);
			}
		} else {
			if(b1 > 0) {
				*Logger::get() << Logger::endl; *Logger::get() << "s"; Logger::get()->hex(b2);
			} else {
				*Logger::get() << "s"; Logger::get()->hex(b2);
			}
		}
		sendUart->send(b1);
		sendUart->send(b2);
	}
}

MdbBridge::MdbBridge(AbstractUart *masterUart, AbstractUart *slaveUart) {
	slaveHandler = new ReceiveHandler(true, slaveUart, masterUart);
	masterHandler = new ReceiveHandler(false, masterUart, slaveUart);
}
#else
#include "MdbBridge.h"
#include "common/mdb/MdbProtocol.h"
#include "common/mdb/MdbProtocolCoinChanger.h"
#include "common/logger/include/Logger.h"

using namespace Mdb::CoinChanger;

MdbCoinChangerSniffer::MdbCoinChangerSniffer() : MdbSlave(Mdb::Device_CoinChanger) {
	static MdbSlavePacketReceiver::Packet packets[] = {
		{ Command_Reset, 0xFF, 0xFF },
		{ Command_Setup, 0xFF, 0xFF },
		{ Command_TubeStatus, 0xFF, 0xFF },
		{ Command_Poll, 0xFF, 0xFF },
		{ Command_CoinType, 0xFF, sizeof(CoinTypeRequest) },
		{ Command_Dispense, 0xFF, sizeof(DispenseRequest) },
		{ Command_Expansion, Subcommand_ExpansionIdentification, 0xFF },
		{ Command_Expansion, Subcommand_ExpansionFeatureEnable, sizeof(ExpansionFeatureEnableRequest) },
	};
	packetLayer = new MdbSlavePacketReceiver(this, packets, sizeof(packets)/sizeof(packets[0]));
}

void MdbCoinChangerSniffer::initSlave(MdbSlave::Sender *sender, MdbSlave::Receiver *receiver) {
	slaveReceiver = receiver;
	packetLayer->init(receiver);
}

void MdbCoinChangerSniffer::reset() {

}

void MdbCoinChangerSniffer::recvCommand(const uint8_t command) {
	packetLayer->recvCommand(command);
}

void MdbCoinChangerSniffer::recvSubcommand(const uint8_t subcommand) {
	packetLayer->recvSubcommand(subcommand);
}

void MdbCoinChangerSniffer::recvRequest(const uint8_t *data, uint16_t len) {
	packetLayer->recvRequest(data, len);
}

void MdbCoinChangerSniffer::recvRequestPacket(const uint16_t commandId, const uint8_t *data, uint16_t len) {
	LOG_DEBUG(LOG_MDBB, "req: " << commandId);
	LOG_DEBUG_HEX(LOG_MDBB, data, len);
}

void MdbCoinChangerSniffer::recvConfirm(uint8_t control) {
	LOG_DEBUG(LOG_MDBB, "confirm: " << control);
}

void MdbCoinChangerSniffer::procResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_DEBUG(LOG_MDBB, "resp:");
	LOG_DEBUG_HEX(LOG_MDBB, data, len);
}

MdbBridge::MdbBridge(AbstractUart *masterUart, AbstractUart *slaveUart) :
	slaveReceiver(this),
	masterReceiver(this),
	coinChangerSniffer(),
	current(NULL)
{
	slaveReceiver.setUart(slaveUart, masterUart);
	masterReceiver.setUart(masterUart, slaveUart);
	coinChangerSniffer.initSlave(NULL, &slaveReceiver);
}

void MdbBridge::recvAddress(const uint8_t address) {
	uint8_t device = address & MDB_DEVICE_MASK;
	if(device != Mdb::Device_CoinChanger) {
		LOG_TRACE(LOG_MDBB, "addr:" << address);
		current = NULL;
		return;
	}
	current = &coinChangerSniffer;
	uint8_t command = address & MDB_COMMAND_MASK;
	current->recvCommand(command);
}

void MdbBridge::recvSubcommand(const uint8_t subcommand) {
	if(current == NULL) { return; }
	current->recvSubcommand(subcommand);
}

void MdbBridge::recvRequest(const uint8_t *data, uint16_t len) {
	if(current == NULL) { return; }
	current->recvRequest(data, len);
}

void MdbBridge::recvConfirm(uint8_t control) {
	if(current == NULL) { return; }
	current->recvConfirm(control);
}

void MdbBridge::procResponse(const uint8_t *data, uint16_t len, bool crc) {
	LOG_TRACE(LOG_MDBB, "recv:");
	LOG_TRACE_HEX(LOG_MDBB, data, len);
	if(current == NULL) { return; }
	((MdbCoinChangerSniffer*)current)->procResponse(data, len, crc);
}

#endif
