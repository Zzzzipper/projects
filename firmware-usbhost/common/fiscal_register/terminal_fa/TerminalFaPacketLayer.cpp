#include "TerminalFaPacketLayer.h"
#include "TerminalFaProtocol.h"
#include "logger/include/Logger.h"

namespace TerminalFa {

/*
Структура пакета:
  Байт 0: 0xB6
  Байт 1: 0x29
  Байт 2,3: длина данных. Little Endian. В длину сообщения не включаются байты префикса (0,1), длины (2,3) и контрольной суммы;
  Байт 4: код команды или ответа – ДВОИЧНОЕ число;
  Байты 5...N: параметры, зависящие от команды (могут отсутствовать);
  Байт N+1,N+2 – контрольная сумма сообщения. Big Endian.

Автомат работы транспорта пакетов:
  документ: Y:\modem\KKT\paykiosk\Допматериалы\ПротоколККТ_ФН_2.0(c24).pdf
  глава: Приложение 3 Рекомендуемая диаграмма состояний обмена стандартного нижнего уровня со стороны ПК

TODO: 1. Выяснить как работать со сменами. Открывать смену раз в день или на каждый чек?
*/
PacketLayer::PacketLayer(TimerEngine *timers, AbstractUart *uart) :
	timers(timers),
	uart(uart),
	observer(NULL),
	state(State_Idle)
{
	this->recvBuf = new Buffer(TERMINALFA_PACKET_MAX_SIZE);
	this->timer = timers->addTimer<PacketLayer, &PacketLayer::procTimer>(this);
	this->uart->setReceiveHandler(this);
}

PacketLayer::~PacketLayer() {
	this->timers->deleteTimer(this->timer);
	delete recvBuf;
}

void PacketLayer::setObserver(Observer *observer) {
	this->observer = observer;
}

bool PacketLayer::sendPacket(Buffer *data) {
	LOG_DEBUG(LOG_FR, "sendPacket");
	Header hdr;
	hdr.fb = Control_FB;
	hdr.sb = Control_SB;
	hdr.len.set(data->getLen());
	sendData((uint8_t*)(&hdr), sizeof(hdr));
	sendData(data->getData(), data->getLen());
	sendCrc(&hdr, data->getData(), data->getLen());
	gotoStateRecvHeader();
	return true;
}

void PacketLayer::sendData(uint8_t *data, uint16_t dataLen) {
	for(uint16_t i = 0; i < dataLen; i++) {
		uart->send(data[i]);
	}
}

void PacketLayer::sendCrc(Header *header, uint8_t *data, uint16_t dataLen) {
	Crc crc;
	crc.start();
	crc.add(header->len.value[0]);
	crc.add(header->len.value[1]);
	crc.add(data, dataLen);
	uart->send(crc.getLowByte());
	uart->send(crc.getHighByte());
}

void PacketLayer::skipData() {
	while(uart->isEmptyReceiveBuffer() == false) {
		uart->receive();
	}
}

void PacketLayer::handle() {
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_FR, "have data " << state);
		switch(state) {
		case State_RecvHeader: stateRecvHeaderRecv(); break;
		case State_RecvData: stateRecvDataRecv(); break;
		case State_SkipData: stateSkipRecv(); break;
		default: LOG_ERROR(LOG_FR, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void PacketLayer::procTimer() {
	LOG_DEBUG(LOG_FR, "procTimer " << state);
	switch(state) {
	case State_RecvHeader: procRecvError(Error_ResponseTimeout); break;
	case State_RecvData: procRecvError(Error_ResponseTimeout); break;
	case State_SkipData: stateSkipTimeout(); break;
	default: LOG_ERROR(LOG_FR, "Unwaited timeout state=" << state);
	}
}

void PacketLayer::gotoStateRecvHeader() {
	LOG_DEBUG(LOG_FR, "gotoStateRecvHeader");
	recvBuf->clear();
	recvLength = sizeof(Header);
	timer->start(TERMINALFA_RESPONSE_TIMER);
	state = State_RecvHeader;
}

void PacketLayer::stateRecvHeaderRecv() {
	LOG_DEBUG(LOG_FR, "stateRecvHeaderRecv");
	uint8_t b1 = uart->receive();
	recvBuf->addUint8(b1);
	if(recvBuf->getLen() < recvLength) {
		return;
	}

	Header *hdr = (Header*)recvBuf->getData();
	if(hdr->fb != Control_FB) {
		LOG_ERROR(LOG_FR, "Wrong first byte");
		gotoStateSkip(Error_PacketFormat);
		return;
	}
	if(hdr->sb != Control_SB) {
		LOG_ERROR(LOG_FR, "Wrong second byte");
		gotoStateSkip(Error_PacketFormat);
		return;
	}
	recvLength = sizeof(Header) + hdr->len.get() + 2;
	if(recvLength > TERMINALFA_PACKET_MAX_SIZE) {
		LOG_ERROR(LOG_FR, "Wrong length");
		gotoStateSkip(Error_PacketFormat);
		return;
	}

	gotoStateRecvData();
}

void PacketLayer::gotoStateRecvData() {
	LOG_DEBUG(LOG_FR, "gotoStateRecvData");
	state = State_RecvData;
}

void PacketLayer::stateRecvDataRecv() {
	LOG_DEBUG(LOG_FR, "stateRecvDataRecv");
	uint8_t b1 = uart->receive();
	recvBuf->addUint8(b1);
	if(recvBuf->getLen() < recvLength) {
		return;
	}

	Header *hdr = (Header*)recvBuf->getData();
	recvLength = hdr->len.get();

	Crc crc;
	crc.start();
	crc.add(hdr->len.value[0]);
	crc.add(hdr->len.value[1]);
	crc.add(hdr->data, recvLength);

	uint8_t crcLowByte = hdr->data[recvLength];
	uint8_t crcHighByte = hdr->data[recvLength+1];
	if((crc.getLowByte() != crcLowByte) || (crc.getHighByte() != crcHighByte)) {
		LOG_ERROR(LOG_FR, "Wrong CRC");
		gotoStateSkip(Error_PacketFormat);
		return;
	}

	timer->stop();
	state = State_Idle;
	if(observer == NULL) {
		LOG_ERROR(LOG_FR, "Observer not defined");
		return;
	}
	observer->procRecvData(hdr->data, recvLength);
}

void PacketLayer::gotoStateSkip(Error error) {
	LOG_DEBUG(LOG_FR, "gotoStateSkip " << error);
	recvError = error;
	state = State_SkipData;
}

void PacketLayer::stateSkipRecv() {
	LOG_DEBUG(LOG_FR, "stateSkipRecv");
	uart->receive();
}

void PacketLayer::stateSkipTimeout() {
	LOG_DEBUG(LOG_FR, "stateSkipTimeout");
	procRecvError(recvError);
}

void PacketLayer::procRecvError(Error error) {
	state = State_Idle;
	if(observer == NULL) {
		LOG_ERROR(LOG_FR, "Observer not defined");
		return;
	}
	observer->procRecvError(error);
}

}
