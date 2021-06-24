#include "SberbankPrinter.h"

#include "logger/include/Logger.h"

namespace Sberbank {

Printer::Printer(
	PacketLayerInterface *packetLayer
) :
	packetLayer(packetLayer),
	state(State_Idle),
	sendBuf(SBERBANK_PACKET_SIZE/2)
{
}

void Printer::procRequest(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECL, "Printer");
	switch(req->instruction) {
		case Instruction_Open: deviceLanOpen(req); return;
		case Instruction_Write: deviceLanWrite(req); return;
		case Instruction_Close: deviceLanClose(req); return;
		default: LOG_ERROR(LOG_SM, "Unwaited intstruction " << req->instruction);
	}
}

void Printer::proc(Event *event) {
	LOG_DEBUG(LOG_SM, "proc " << state);
	switch(state) {
		default: LOG_ERROR(LOG_SM, "Unwaited event " << state << "," << event->getType());
	}
}

void Printer::deviceLanOpen(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECL, "deviceLanOpen");
//	DevicePrinterOpenParam *param = (DevicePrinterOpenParam*)(req->paramVal);
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->result = 0;
	resp->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	resp->instruction = req->instruction;
	resp->device = req->device;
	resp->reserved = 0;
	resp->paramLen.set(0);
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet));
	sendBuf.setLen(sizeof(MasterCallRequest));
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
}

void Printer::deviceLanWrite(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECL, "deviceLanWrite");
//	DevicePrinterSendParam *param = (DevicePrinterSendParam*)(req->paramVal);
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->result = 0;
	resp->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	resp->instruction = req->instruction;
	resp->device = req->device;
	resp->reserved = 0;
	resp->paramLen.set(0);
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet));
	sendBuf.setLen(sizeof(MasterCallRequest));
	packetLayer->sendPacket(&sendBuf);
	state = State_Wait;
}

void Printer::deviceLanClose(MasterCallRequest *req) {
	LOG_DEBUG(LOG_ECL, "deviceLanClose");
	MasterCallRequest *resp = (MasterCallRequest *)(sendBuf.getData());
	resp->result = 0;
	resp->num.set(req->num.get() | SBERBANK_RESPONSE_FLAG);
	resp->instruction = req->instruction;
	resp->device = req->device;
	resp->reserved = 0;
	resp->paramLen.set(0);
	resp->len.set(sizeof(MasterCallRequest) - sizeof(Packet));
	sendBuf.setLen(sizeof(MasterCallRequest));
	packetLayer->sendPacket(&sendBuf);
	state = State_Idle;
}

}
