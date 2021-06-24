#include "DdcmpCommandLayer.h"
#include "DdcmpProtocol.h"

#include "logger/include/Logger.h"

namespace Ddcmp {

CommandLayer::CommandLayer(PacketLayerInterface *packetLayer, TimerEngine *timers) :
	packetLayer(packetLayer),
	timers(timers)
{
	this->packetLayer->setObserver(this);
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
}

CommandLayer::~CommandLayer() {
	timers->deleteTimer(this->timer);
}

void CommandLayer::recvAudit(Dex::DataParser *dataParser, Dex::CommandResult *commandResult) {
	LOG_INFO(LOG_DDCMP, "recvAudit");
	this->dataParser = dataParser;
	this->commandResult = commandResult;
	packetLayer->reset();
	gotoStateBaudRate();
}


void CommandLayer::recvControl(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_DDCMP, "recvControl");
	LOG_DEBUG_HEX(LOG_DDCMP, data, dataLen);
	switch(state) {
	case State_BaudRate: stateBaudRatePacket(data, dataLen); break;
	case State_Auth: stateAuthControl(data, dataLen); break;
	case State_AuditStart: stateAuditStartControl(data, dataLen); break;
	case State_Finish: stateFinishControl(data, dataLen); break;
	default: LOG_ERROR(LOG_DDCMP, "Unwaited data " << state);
	}
}

void CommandLayer::recvData(uint8_t *cmd, uint16_t cmdLen, uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_DDCMP, "recvData");
	switch(state) {
	case State_AuthResponse: stateAuthResponseData(cmd, cmdLen, data, dataLen); break;
	case State_AuditStartResponse: stateAuditStartResponseData(cmd, cmdLen, data, dataLen); break;
	case State_AuditRecv: stateAuditRecvData(cmd, cmdLen, data, dataLen); break;
	default: LOG_ERROR(LOG_DDCMP, "Unwaited data " << state);
	}
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_DDCMP, "procTimer");
/*	switch(state) {
	case State_Session: stateSessionTimeout(); break;
	case State_Request: stateRequestTimeout(); break;
	case State_Approving: stateApprovingTimeout(); break;
	case State_PaymentCancel: statePaymentCancelTimeout(); break;
	case State_PaymentCancelWait: statePaymentCancelWaitTimeout(); break;
	case State_Closing: stateClosingTimeout(); break;
	case State_QrCode: stateQrCodeTimeout(); break;
	case State_QrCodeWait: stateQrCodeWaitTimeout(); break;
	default: LOG_ERROR(LOG_DDCMP, "Unwaited timeout " << state);
	}*/
}

void CommandLayer::gotoStateBaudRate() {
	LOG_DEBUG(LOG_DDCMP, "gotoStateBaudRate");
	Control command;
	command.message = Message_Control;
	command.type = Type_Start;
	command.flags = 0x40;
	command.sbd = 0;
	command.mdr = BaudRate_115200;
	command.sadd = 1;
	packetLayer->sendControl((uint8_t*)&command, sizeof(command));
	sendCnt = 1;
	receiveCnt = 0;
	state = State_BaudRate;
}

void CommandLayer::stateBaudRatePacket(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_DDCMP, "stateBaudRatePacket");
	Control *command = (Control*)data;
	if(command->message != Message_Control || command->type != Type_Stack) {
		LOG_ERROR(LOG_DDCMP, "Wait STACK, but receive " << command->type);
		return;
	}

	LOG_INFO(LOG_DDCMP, "BaudRate " << command->mdr);
	gotoStateAuth();
}

void CommandLayer::gotoStateAuth() {
	LOG_DEBUG(LOG_DDCMP, "gotoStateAuth");
	uint16_t size = 16;
	Control command;
	command.message = Message_Data;
	command.type = size;
	command.flags = size >> 8 & 0xff;
	command.sbd = receiveCnt;
	command.mdr = sendCnt;
	command.sadd = 1;
	sendCnt++;

	WhoAreYouRequest data;
	data.message = Message_Request;
	data.type =Type_WhoAreYou;
	data.zero1 = 0;
	data.securityCode.set(0);
	data.passCode.set(0);
	data.dateDay.set(1);
	data.dateMonth.set(3);
	data.dateYear.set(19);
	data.timeHour.set(20);
	data.timeMinute.set(19);
	data.timeSecond.set(17);
	data.userId.set(0);
	data.flag = Flag_RoutePerson;

	packetLayer->sendData((uint8_t*)&command, sizeof(command), (uint8_t*)&data, sizeof(data));
	state = State_Auth;
}

void CommandLayer::stateAuthControl(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_DDCMP, "stateAuthPacket");
	Control *command = (Control*)data;
	if(command->message != Message_Control || command->type != Type_Ack) {
		LOG_ERROR(LOG_DDCMP, "Wait STACK, but receive " << command->type);
		return;
	}

	LOG_INFO(LOG_DDCMP, "ACK");
	gotoStateAuthResponse();
}

void CommandLayer::gotoStateAuthResponse() {
	LOG_DEBUG(LOG_DDCMP, "gotoStateAuthResponse");
	state = State_AuthResponse;
}

void CommandLayer::stateAuthResponseData(const uint8_t *cmd, uint16_t cmdLen, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_DDCMP, "stateAuthResponsePacket");
	Control *command = (Control*)cmd;
	if(command->message != Message_Data) {
		LOG_ERROR(LOG_DDCMP, "Wait AuthResponse, but receive " << command->type);
		return;
	}
	receiveCnt = command->mdr;

	WhoAreYouResponse *resp = (WhoAreYouResponse*)data;
	LOG_DEBUG(LOG_DDCMP, "message=" << resp->message);
	LOG_DEBUG(LOG_DDCMP, "type=" << resp->type);
	LOG_DEBUG(LOG_DDCMP, "securityCode=" << resp->securityCode.get());
	LOG_DEBUG(LOG_DDCMP, "passCode=" << resp->passCode.get());
	LOG_DEBUG(LOG_DDCMP, "softwareVersion=" << resp->softwareVersion);
	LOG_DEBUG(LOG_DDCMP, "manufacturer=" << resp->manufacturer);
	LOG_DEBUG(LOG_DDCMP, "extraRead=" << resp->extraRead);
	LOG_DEBUG(LOG_DDCMP, "msdb=" << resp->msdb);
	LOG_DEBUG(LOG_DDCMP, "lsdb=" << resp->lsdb);
	stateAuthResponseAck();

	gotoStateAuditStart();
}

//050140020001
void CommandLayer::stateAuthResponseAck() {
	LOG_DEBUG(LOG_DDCMP, "stateAuthResponseAck");
	Control command;
	command.message = Message_Control;
	command.type = Type_Ack;
	command.flags = 0x40;
	command.sbd = receiveCnt;
	command.mdr = 0;
	command.sadd = 1;
	packetLayer->sendControl((uint8_t*)&command, sizeof(command));
}

/*
"<command=810900010201"
",data=77E20001010000FFFF>", packetLayer->getSendData());
 */
void CommandLayer::gotoStateAuditStart() {
	LOG_DEBUG(LOG_DDCMP, "gotoStateAuditStart");
	uint16_t size = sizeof(StartRequest);
	Control command;
	command.message = Message_Data;
	command.type = size;
	command.flags = size >> 8 & 0xff;
	command.sbd = receiveCnt;
	command.mdr = sendCnt;
	command.sadd = 1;
	sendCnt++;

	StartRequest data;
	data.message = Message_Request;
	data.type =Type_Read;
	data.subtype = 0;
	data.listNumber = ListNumber_AuditOnly;
	data.recordNumber = Constant_RecordNumber;
	data.byteOffset.set(0);
	data.segmentLength.set(0xFFFF);

	packetLayer->sendData((uint8_t*)&command, sizeof(command), (uint8_t*)&data, sizeof(data));
	state = State_AuditStart;
}

void CommandLayer::stateAuditStartControl(const uint8_t *cmd, uint16_t cmdLen) {
	LOG_DEBUG(LOG_DDCMP, "stateAuditStartControl");
	Control *command = (Control*)cmd;
	if(command->message != Message_Control || command->type != Type_Ack) {
		LOG_ERROR(LOG_DDCMP, "Wait STACK, but receive " << command->type);
		return;
	}

	LOG_INFO(LOG_DDCMP, "ACK");
	gotoStateAuditStartResponse();
}

void CommandLayer::gotoStateAuditStartResponse() {
	LOG_DEBUG(LOG_DDCMP, "gotoStateAuditStartResponse");
	state = State_AuditStartResponse;
}

void CommandLayer::stateAuditStartResponseData(const uint8_t *cmd, uint16_t cmdLen, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_DDCMP, "stateAuditStartResponseData");
	ReadControl *command = (ReadControl*)cmd;
	if(command->message != Message_Data) {
		LOG_ERROR(LOG_DDCMP, "Wait AuthResponse, but receive " << command->message);
		return;
	}
	LOG_DEBUG(LOG_DDCMP, "cmdLen=" << cmdLen);
	LOG_DEBUG(LOG_DDCMP, "message=" << command->message);
	LOG_DEBUG(LOG_DDCMP, "len=" << command->len);
	LOG_DEBUG(LOG_DDCMP, "flags=" << command->flags);
	LOG_DEBUG(LOG_DDCMP, "rx=" << command->rx);
	LOG_DEBUG(LOG_DDCMP, "tx=" << command->tx);
	LOG_DEBUG(LOG_DDCMP, "sadd=" << command->sadd);
	receiveCnt = command->tx;

	StartResponse *resp = (StartResponse*)data;
	LOG_DEBUG(LOG_DDCMP, "message=" << resp->message);
	LOG_DEBUG(LOG_DDCMP, "type=" << resp->type);
	LOG_DEBUG(LOG_DDCMP, "subtype=" << resp->subtype);
	LOG_DEBUG(LOG_DDCMP, "listNumber=" << resp->listNumber);
	LOG_DEBUG(LOG_DDCMP, "recordNumber=" << resp->recordNumber);
	LOG_DEBUG(LOG_DDCMP, "byteOffset=" << resp->byteOffset.get());
	LOG_DEBUG(LOG_DDCMP, "segmentLength=" << resp->segmentLength.get());

	dataParser->start(resp->segmentLength.get());
	stateAuditRecvAck();
	gotoStateAuditRecv();
}

void CommandLayer::gotoStateAuditRecv() {
	LOG_DEBUG(LOG_DDCMP, "gotoStateAuditRecv");
	state = State_AuditRecv;
}

void CommandLayer::stateAuditRecvData(const uint8_t *cmd, uint16_t cmdLen, const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_DDCMP, "stateAuditRecvData");
	ReadControl *command = (ReadControl*)cmd;
	if(command->message != Message_Data) {
		LOG_ERROR(LOG_DDCMP, "Wait AuthResponse, but receive " << command->message);
		return;
	}
	LOG_DEBUG(LOG_DDCMP, "cmdLen=" << cmdLen);
	LOG_DEBUG(LOG_DDCMP, "message=" << command->message);
	LOG_DEBUG(LOG_DDCMP, "len=" << command->len);
	LOG_DEBUG(LOG_DDCMP, "flags=" << command->flags);
	LOG_DEBUG(LOG_DDCMP, "rx=" << command->rx);
	LOG_DEBUG(LOG_DDCMP, "tx=" << command->tx);
	LOG_DEBUG(LOG_DDCMP, "sadd=" << command->sadd);
	receiveCnt = command->tx;

	ReadResponse *resp = (ReadResponse*)data;
	uint16_t readResponseSize = sizeof(ReadResponse);
	LOG_DEBUG(LOG_DDCMP, "dataLen=" << dataLen);
	LOG_DEBUG(LOG_DDCMP, "dataLen2=" << (dataLen - readResponseSize));
	LOG_DEBUG(LOG_DDCMP, "wtf=" << resp->wtf);
	LOG_DEBUG(LOG_DDCMP, "num=" << resp->num);
	LOG_DEBUG_HEX(LOG_DDCMP, data + readResponseSize, dataLen - readResponseSize);
	dataParser->procData(data + readResponseSize, dataLen - readResponseSize);

	if((command->flags & DataFlag_LastBlock) == 0) {
		stateAuditRecvAck();
		gotoStateAuditRecv();
	} else {
		stateAuditRecvAck();
		gotoStateFinish();
	}
}

//050140020001
void CommandLayer::stateAuditRecvAck() {
	LOG_DEBUG(LOG_DDCMP, "stateAuditRecvAck");
	Control command;
	command.message = Message_Control;
	command.type = Type_Ack;
	command.flags = 0x40;
	command.sbd = receiveCnt;
	command.mdr = 0;
	command.sadd = 1;
	packetLayer->sendControl((uint8_t*)&command, sizeof(command));
}

void CommandLayer::gotoStateFinish() {
	LOG_DEBUG(LOG_DDCMP, "gotoStateFinish");
	uint16_t size = sizeof(FinishRequest);
	Control command;
	command.message = Message_Data;
	command.type = size;
	command.flags = size >> 8 & 0xff;
	command.sbd = receiveCnt;
	command.mdr = sendCnt;
	command.sadd = 1;
	sendCnt++;

	FinishRequest data;
	data.message = Message_Request;
	data.type = Constant_Finish;

	packetLayer->sendData((uint8_t*)&command, sizeof(command), (uint8_t*)&data, sizeof(data));
	state = State_Finish;
}

void CommandLayer::stateFinishControl(const uint8_t *cmd, uint16_t cmdLen) {
	LOG_DEBUG(LOG_DDCMP, "stateFinishControl");
	Control *command = (Control*)cmd;
	if(command->message != Message_Control || command->type != Type_Ack) {
		LOG_ERROR(LOG_DDCMP, "Wait ACK, but receive " << command->type);
		return;
	}

	LOG_INFO(LOG_DDCMP, "ACK");
	dataParser->complete();
}

}
