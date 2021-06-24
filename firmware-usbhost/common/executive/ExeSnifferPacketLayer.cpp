#include "ExeSnifferPacketLayer.h"
#include "ExeProtocol.h"

#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"

using namespace Exe;

//#define SNIF_EXECUTIVE

ExeSnifferPacketLayer::ExeSnifferPacketLayer(Customer *customer, StatStorage *stat) :
	state(State_Byte),
	slaveReceiver(this),
	masterReceiver(this),
	customer(customer),
	req(0),
	data(EXE_PACKET_MAX_SIZE)
{
	this->countCrcError = stat->add(Mdb::DeviceContext::Info_ExeM_CrcErrorCount, 0);
	this->countResponse = stat->add(Mdb::DeviceContext::Info_ExeM_ResponseCount, 0);
}

ExeSnifferPacketLayer::~ExeSnifferPacketLayer() {

}

void ExeSnifferPacketLayer::init(AbstractUart *slaveUart, AbstractUart *masterUart) {
	LOG_INFO(LOG_EXE, "init");
	slaveReceiver.setUart(slaveUart);
	masterReceiver.setUart(masterUart);
}

void ExeSnifferPacketLayer::recvRequest(uint8_t req) {
	this->req = req;
	uint8_t commandFlag = req & Flag_Command;
	uint8_t command = req & Mask_Data;
	if(commandFlag > 0) {
		if(command == Command_Vend) {
			Logger *logger = Logger::get();
			*logger << " VEND !!!!!!!!" << Logger::datetime << Logger::endl;
		}
	}
}

void ExeSnifferPacketLayer::recvResponse(uint8_t resp) {
#ifdef SNIF_EXECUTIVE
	printPair(req, resp);
#endif
	uint8_t address = req & Mask_Device;
	if(address != Device_VMC) {
		LOG_DEBUG(LOG_EXE, "Ignore pair: req=" << req << ", resp=" << resp << ", wrong device");
		return;
	}
	if(resp == ResponseCode_Negative) {
		LOG_DEBUG(LOG_EXE, "Ignore pair: req=" << req << ", resp=" << resp << ", transfer error");
		return;
	}
	switch(state) {
	case State_Byte: stateByteRecv(req, resp); break;
	case State_Data: stateDataRecv(req, resp); break;
	case State_DataSync: stateDataSyncRecv(req, resp); break;
	default: LOG_ERROR(LOG_EXE, "Wrong state " << state);
	}
}

void ExeSnifferPacketLayer::stateByteRecv(uint8_t req, uint8_t resp) {
	if((req & Flag_Command) == false) {
		LOG_DEBUG(LOG_EXE, "Ignore pair: req=" << req << ", resp=" << resp << ", unwaited data");
		return;
	}
	uint8_t command = req & Mask_Data;
	if(command == Command_AcceptData) {
		data.clear();
		state = State_Data;
		return;
	} else {
		countResponse->inc();
		customer->recvByte(command, resp);
	}
}

void ExeSnifferPacketLayer::stateDataRecv(uint8_t req, uint8_t resp) {
	if((req & Flag_Command) == true) {
		LOG_DEBUG(LOG_EXE, "Ignore pair: req=" << req << ", resp=" << resp << ", wrong command flag");
		return;
	}
	data.addUint8(req);
	if(data.getLen() >= EXE_PACKET_MAX_SIZE) {
		state = State_DataSync;
	}
}

void ExeSnifferPacketLayer::stateDataSyncRecv(uint8_t req, uint8_t resp) {
	if((req & Flag_Command) == false) {
		LOG_DEBUG(LOG_EXE, "Ignore pair: req=" << req << ", resp=" << resp << ", unwaited data");
		return;
	}
	uint8_t command = req & Mask_Data;
	if(command == Command_DataSync) {
		countResponse->inc();
		state = State_Byte;
		customer->recvData(data.getData(), data.getLen());
	}
}

#ifdef SNIF_EXECUTIVE
#include "timer/stm32/include/SystemTimer.h"

void ExeSnifferPacketLayer::printPair(uint8_t req, uint8_t resp) {
	uint32_t tms = SystemTimer::get()->getMs();
	Logger *logger = Logger::get();
	*logger << tms << "<<";
	logger->hex(req);
	*logger << "->";
	logger->hex(resp);

	if(resp == ResponseCode_Negative) {
		*logger << " NAK";
		*logger << Logger::endl;
		return;
	}

	uint8_t address = req & Mask_Device;
	uint8_t commandFlag = req & Flag_Command;
	uint8_t command = req & Mask_Data;
	if(commandFlag > 0) {
		switch(command) {
		case Command_Status: *logger << " STATUS"; break;
		case Command_Credit: {
			*logger << " CREDIT";
			if(resp < 250) { *logger << " !!!!" << Logger::datetime << resp; }
			break;
		}
		case Command_Vend: *logger << " VEND " << resp << " !!!!!!!! " << Logger::datetime; break;
		case Command_AcceptData: *logger << " AD"; break;
		case Command_DataSync: *logger << " DS " << Logger::datetime; break;
		default: *logger << " UNKNOWN";
		}
	}
	*logger << Logger::endl;
}
#endif
