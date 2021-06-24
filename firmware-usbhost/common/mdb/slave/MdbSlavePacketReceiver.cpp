#include "MdbSlavePacketReceiver.h"
#include "logger/include/Logger.h"

MdbSlavePacketReceiver::MdbSlavePacketReceiver(uint8_t device, MdbSlave::PacketObserver *observer, Packet *desc, uint8_t descNum) :
	observer(observer),
	desc(desc),
	descNum(descNum),
	device(device),
	mode(Mdb::Mode_Normal)
{
	LOG_INFO(LOG_MDBS, "Registered " << descNum << " packets");
}

void MdbSlavePacketReceiver::init(MdbSlave::Receiver *receiver) {
	this->receiver = receiver;
}

void MdbSlavePacketReceiver::setMode(uint8_t mode) {
	this->mode = mode;
}

uint8_t MdbSlavePacketReceiver::getMode() {
	return mode;
}

void MdbSlavePacketReceiver::recvCommand(const uint8_t command) {
	LOG_INFO(LOG_MDBS, "recvCommand " << device << "," << command);
	this->command = command;
	for(uint8_t i = 0; i < descNum; i++) {
		if(desc[i].command == command) {
			if(desc[i].subcommand < MDB_SUBCOMMAND_NONE) {
				waitSubcommand();
				return;
			} else {
				if(mode == Mdb::Mode_Normal) {
					if(desc[i].len < 0xFF) {
						descIndex = i;
						waitRequest(desc[i].len);
						return;
					} else {
						descIndex = i;
						waitRequest(1);
						return;
					}
				} else {
					if(desc[i].expandedLen < 0xFF) {
						descIndex = i;
						waitRequest(desc[i].expandedLen);
						return;
					} else {
						descIndex = i;
						waitRequest(1);
						return;
					}
				}
			}
		}
	}
	LOG_ERROR(LOG_MDBS, "Unsupported command " << device << "," << command);
	observer->recvUnsupportedPacket(command << 8);
}

void MdbSlavePacketReceiver::recvSubcommand(const uint16_t subcommand) {
	LOG_INFO(LOG_MDBS, "recvSubcommand " << device << "," << mode << "," << command << "," << subcommand);
	for(uint8_t i = 0; i < descNum; i++) {
		if(desc[i].command == command && desc[i].subcommand == subcommand) {
			if(mode == Mdb::Mode_Normal) {
				if(desc[i].len < 0xFF) {
					descIndex = i;
					waitRequest(desc[i].len);
					return;
				} else {
					descIndex = i;
					waitRequest(2);
					return;
				}
			} else {
				if(desc[i].expandedLen < 0xFF) {
					descIndex = i;
					waitRequest(desc[i].expandedLen);
					return;
				} else {
					descIndex = i;
					waitRequest(2);
					return;
				}
			}
		}
	}
	LOG_ERROR(LOG_MDBS, "Unsupported subcommand " << device << "," << command << "," << subcommand);
	observer->recvUnsupportedPacket((command << 8) | (subcommand & 0xFF));
}

void MdbSlavePacketReceiver::recvRequest(const uint8_t *data, uint16_t len) {
	LOG_INFO(LOG_MDBS, "recvRequest" << device);
	LOG_INFO_HEX(LOG_MDBS, data, len);
	uint16_t commandId = desc[descIndex].command << 8;
	if(desc[descIndex].subcommand < MDB_SUBCOMMAND_NONE) { commandId |= desc[descIndex].subcommand; }
	observer->recvRequestPacket(commandId, data, len);
}

void MdbSlavePacketReceiver::waitSubcommand() {
	LOG_INFO(LOG_MDBS, "waitSubcommand" << device);
	receiver->recvSubcommand();
}

void MdbSlavePacketReceiver::waitRequest(uint16_t len) {
	LOG_INFO(LOG_MDBS, "waitRequest " << device << "," << len);
	receiver->recvRequest(len);
}
