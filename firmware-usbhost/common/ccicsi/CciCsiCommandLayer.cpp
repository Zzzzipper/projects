#include "CciCsiCommandLayer.h"
#include "CciCsiProtocol.h"

#include "mdb/slave/cashless/MdbSlaveCashless3.h"
#include "utils/include/DecimalPoint.h"
#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

#include <string.h>

namespace CciCsi {

CommandLayer::CommandLayer(PacketLayerInterface *packetLayer, TimerEngine *timers, EventEngineInterface *eventEngine) :
	packetLayer(packetLayer),
	timers(timers),
	eventEngine(eventEngine),
	deviceId(eventEngine),
	state(State_Idle),
	response(CCICSI_PACKET_MAX_SIZE)
{
//	this->context->setStatus(Mdb::DeviceContext::Status_NotFound);
//	this->context->setState(State_Idle);
	this->packetLayer->setObserver(this);
	this->timer = timers->addTimer<CommandLayer, &CommandLayer::procTimer>(this);
}

CommandLayer::~CommandLayer() {
	timers->deleteTimer(this->timer);
}

EventDeviceId CommandLayer::getDeviceId() {
	return deviceId;
}

void CommandLayer::reset() {
	LOG_INFO(LOG_CCI, "reset");
	packetLayer->reset();
	state = State_Wait;
}

bool CommandLayer::isInited() {
	return false;
}

bool CommandLayer::isEnable() {
	return false;
}

void CommandLayer::setCredit(uint32_t credit) {
	LOG_INFO(LOG_CCI, "setCredit");
	if(state == State_Wait) {
		this->credit = credit;
		this->state = State_Credit;
	}
}

void CommandLayer::approveVend(uint32_t ) {
	LOG_INFO(LOG_CCI, "approveVend");
	if(state == State_Approving) {
		state = State_Approve;
	}
}

void CommandLayer::denyVend() {
	LOG_INFO(LOG_CCI, "denyVend");
	if(state == State_Approving) {
		state = State_Wait;
	}
}

void CommandLayer::cancelVend() {
	LOG_INFO(LOG_CCI, "cancelVend");
	state = State_Wait;
}

void CommandLayer::procTimer() {
	LOG_DEBUG(LOG_CCI, "procTimer");

}

void CommandLayer::procPacket(const uint8_t *data, const uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "procPacket");
	LOG_INFO_HEX(LOG_CCI, data, dataLen);
	switch(state) {
	case State_Wait: stateWaitRequest(data, dataLen); break;
	case State_Credit: stateCreditRequest(data, dataLen); break;
	case State_Approving: stateApprovingRequest(data, dataLen); break;
	case State_Approve: stateApproveRequest(data, dataLen); break;
	case State_Vending: stateVendingRequest(data, dataLen); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited data " << state);
	}
}

void CommandLayer::stateWaitRequest(const uint8_t *data, uint16_t ) {
	LOG_DEBUG(LOG_CCI, "stateWaitRequest");
	packetLayer->sendControl(Control_ACK);
	Header *hdr = (Header*)data;
	switch(hdr->type) {
	case Operation_Identification: sendIdentification(); break;
	case Operation_Status: sendStatusWait(); break;
	case Operation_Inquiry: sendVendDenied(); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateCreditRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateCreditRequest");
	packetLayer->sendControl(Control_ACK);
	Header *hdr = (Header*)data;
	switch(hdr->type) {
	case Operation_Identification: sendIdentification(); break;
	case Operation_Status: sendStatusCredit(); break;
	case Operation_Inquiry: stateCreditRequestInquiry(data, dataLen); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateCreditRequestInquiry(const uint8_t *data, uint16_t ) {
	InquiryRequest *req = (InquiryRequest*)data;
	sendVendDenied();
	productId = req->articleNumber.get();
	gotoStateApproving();
	MdbSlaveCashlessInterface::EventVendRequest event(deviceId, MdbSlaveCashlessInterface::Event_VendRequest, productId, 0);
	eventEngine->transmit(&event);
}

void CommandLayer::gotoStateApproving() {
	LOG_DEBUG(LOG_CCI, "gotoStateApproving");
	statusCount = 0;
	state = State_Approving;
}

void CommandLayer::stateApprovingRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateApprovingRequest");
	packetLayer->sendControl(Control_ACK);
	Header *hdr = (Header*)data;
	switch(hdr->type) {
	case Operation_Identification: sendIdentification(); break;
	case Operation_Status: stateApprovingRequestStatus(); break;
	case Operation_Inquiry: stateApprovingRequestInquiry(data, dataLen); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateApprovingRequestStatus() {
	sendStatusCredit();
	statusCount++;
	if(statusCount >= CCICSI_STATUS_MAX) {
		state = State_Wait;
		EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendCancel);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateApprovingRequestInquiry(const uint8_t *data, uint16_t ) {
	InquiryRequest *req = (InquiryRequest*)data;
	sendVendDenied();
	statusCount = 0;
	if(productId != req->articleNumber.get()) {
		state = State_Wait;
		EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendCancel);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateApproveRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateVendApproveRequest");
	packetLayer->sendControl(Control_ACK);
	Header *hdr = (Header*)data;
	switch(hdr->type) {
	case Operation_Identification: sendIdentification(); break;
	case Operation_Status: sendStatusCredit(); break;
	case Operation_Inquiry: stateApproveRequsetInquiry(data, dataLen); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateApproveRequsetInquiry(const uint8_t *data, uint16_t ) {
	InquiryRequest *req = (InquiryRequest*)data;
	if(productId == req->articleNumber.get()) {
		sendVendApproved();
		state = State_Vending;
		return;
	} else {
		sendVendDenied();
		state = State_Wait;
		EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendCancel);
		eventEngine->transmit(&event);
		return;
	}
}

void CommandLayer::stateVendingRequest(const uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_CCI, "stateVendingRequest");
	packetLayer->sendControl(Control_ACK);
	Header *hdr = (Header*)data;
	switch(hdr->type) {
	case Operation_Identification: sendIdentification(); break;
	case Operation_Status: stateVendingRequestStatus(data, dataLen); break;
	case Operation_Inquiry: sendVendDenied(); break;
	default: LOG_ERROR(LOG_CCI, "Unwaited request " << hdr->type);
	}
}

void CommandLayer::stateVendingRequestStatus(const uint8_t *data, uint16_t ) {
	InquiryRequest *req = (InquiryRequest*)data;
	sendStatusCredit();
	state = State_Credit;
	EventInterface event(deviceId, MdbSlaveCashlessInterface::Event_VendComplete);
	eventEngine->transmit(&event);
}

void CommandLayer::sendIdentification() {
	IdentificationResponse *resp = (IdentificationResponse*)response.getData();
	resp->type = Operation_Identification;
	resp->interface = Interface_CSI;
	resp->paymentSystem.set(T1_MdbCoinChanger);
	resp->version.set(207);
	resp->level.set(4);
	response.setLen(sizeof(IdentificationResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendStatusWait() {
	StatusResponse *resp = (StatusResponse*)response.getData();
	resp->type = Operation_Status;
	resp->status = Status_NoAction;
	resp->IF_STAT = 0x90;
	resp->TO_PS.set(0);
	resp->reserved = 0x00;
	response.setLen(sizeof(StatusResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendStatusCredit() {
	StatusResponse *resp = (StatusResponse*)response.getData();
	resp->type = Operation_Status;
	resp->status = Status_Ready;
	resp->IF_STAT = 0x90;
	resp->TO_PS.set(0);
	resp->reserved = 0x00;
	response.setLen(sizeof(StatusResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendVendApproved() {
	InquiryResponse *resp = (InquiryResponse*)response.getData();
	resp->type = Operation_Inquiry;
	resp->verdict = Verdict_Approve;
	response.setLen(sizeof(InquiryResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::sendVendDenied() {
	InquiryResponse *resp = (InquiryResponse*)response.getData();
	resp->type = Operation_Inquiry;
	resp->verdict = Verdict_Deny;
	response.setLen(sizeof(InquiryResponse));
	packetLayer->sendPacket(&response);
}

void CommandLayer::procControl(uint8_t control) {
	LOG_DEBUG(LOG_CCI, "procControl " << control);

}

void CommandLayer::procError(Error ) {
	LOG_DEBUG(LOG_CCI, "procError");

}

}
