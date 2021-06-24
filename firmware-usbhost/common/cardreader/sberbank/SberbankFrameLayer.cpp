#include "SberbankFrameLayer.h"

#include "SberbankProtocol.h"
#include "BluetoothCrc.h"

#include "utils/include/CodePage.h"
#include "logger/include/Logger.h"

namespace Sberbank {

FrameLayer::FrameLayer(TimerEngine *timers, AbstractUart *uart) :
	timers(timers),
	uart(uart),
	observer(NULL),
	state(State_Idle),
	sendBuf(SBERBANK_FRAME_SIZE),
	recvBuf(SBERBANK_FRAME_SIZE),
	crcBuf(BLUETOOTH_CRC_SIZE)
{
	this->timer = timers->addTimer<FrameLayer, &FrameLayer::procTimer>(this);
	this->uart->setReceiveHandler(this);
}

FrameLayer::~FrameLayer() {
	this->timers->deleteTimer(this->timer);
}

void FrameLayer::setObserver(FrameLayerObserver *observer) {
	this->observer = observer;
}

void FrameLayer::reset() {
	LOG_DEBUG(LOG_ECLF, "reset");
	gotoStateStx();
}

bool FrameLayer::sendPacket(Buffer *data) {
	LOG_DEBUG(LOG_ECLF, "sendPacket");
	uart->send(Control_STX);
	uart->send(Control_MARK);
	uint16_t sendLen = convertBinToBase64(data->getData(), data->getLen(), sendBuf.getData(), sendBuf.getSize());
	sendBuf.setLen(sendLen);
	sendData(sendBuf.getData(), sendBuf.getLen());
	uart->send(Control_ETX);

	LOG_DEBUG(LOG_ECLF, ">> " << data->getLen() << "/" << sendBuf.getLen());
	LOG_TRACE_HEX(LOG_ECLF, Control_STX);
	LOG_TRACE_HEX(LOG_ECLF, Control_MARK);
	LOG_TRACE_HEX(LOG_ECLF, sendBuf.getData(), sendBuf.getLen());
	LOG_TRACE_HEX(LOG_ECLF, Control_ETX);
	return true;
}

/*
x02; // Control_STX
x23; // Control_MARK
x41;x42;x41;x41;x43;x51;x49;x69;x41;x41;x43;x41;x41;x68;x6B;x41;x42;x41;x49;x48;x42;x41;x42;x54;x41;x45;x4A;x48;x55;x77;x46;x4E;x55;x61;x73;x41;x52;x4F;x4E;x51;x41;x67;x41;x41;x41;x42;x34;x41;x61;x51;x41;x41;x41;x41;x49;x42;x4E;x44;x78;x48;x41;x41;x49;x4A;x74;x51;x41;x41;x2F;x6E;x67;x41;x41;x41;x41;x41;x2F;x77;x61;x43;x41;x59;x50;x66;x44;x51;x45;x42;x33;x78;x61;x43;x41;x58;x6F;x69;x41;x41;x41;x41;x49;x67;x53;x5A;x6D;x52;x49;x69;x49;x51;x41;x41;x4A;x79;x43;x5A;x6D;x51;x45;x77;x41;x41;x41;x41;x4D;x4A;x6D;x5A;x6D;x51;x59;x77;x69;x41;x41;x41;x4D;x4A;x53;x5A;x6D;x52;x55;x77;x6C;x67;x41;x41;x4D;x51;x4B;x5A;x6D;x52;x55;x78;x45;x67;x41;x41;x4D;x53;x43;x5A;x6D;x52;x55;x78;x57;x41;x41;x41;x4D;x56;x6D;x5A;x6D;x52;x55;x7A;x4E;x77;x41;x41;x4D;x30;x6D;x5A;x6D;x52;x55;x30;x41;x41;x41;x41;x4E;x4A;x6D;x5A;x6D;x51;x55;x31;x4B;x41;x41;x41;x4E;x59;x6D;x5A;x6D;x52;x55;x32;x41;x41;x41;x41;x4E;x70;x6D;x5A;x6D;x51;x59;x33;x41;x41;x41;x41;x4E;x35;x6D;x5A;x6D;x51;x55;x34;x41;x41;x41;x41;x4F;x5A;x6D;x5A;x6D;x51;x5A;x41;x41;x41;x41;x41;x53;x5A;x6D;x5A;x6D;x51;x4E;x43;x64;x6F;x41;x41;x51;x6E;x61;x4A;x6D;x51;x52;x51;x41;x41;x41;x41;x55;x4A;x6D;x5A;x6D;x51;x4A;x52;x41;x41;x41;x41;x56;x5A;x6D;x5A;x6D;x51;x46;x57;x41;x41;x41;x41;x57;x5A;x6D;x5A;x6D;x51;x4A;x67;x41;x41;x41;x41;x61;x5A;x6D;x5A;x6D;x51;x4A;x67;x45;x51;x41;x41;x59;x42;x45;x4A;x6D;x51;x5A;x67;x45;x53;x41;x41;x59;x42;x46;x4A;x6D;x51;x5A;x67;x45;x58;x51;x41;x59;x42;x46;x30;x6D;x51;x5A;x67;x45;x58;x63;x41;x59;x42;x46;x33;x6D;x51;x5A;x67;x45;x59;x59;x41;x59;x42;x47;x5A;x6D;x51;x5A;x67;x56;x47;x45;x41;x59;x46;x52;x69;x6D;x51;x64;x69;x41;x41;x41;x41;x59;x70;x6D;x5A;x6D;x51;x35;x6B;x51;x41;x41;x41;x5A;x5A;x6D;x5A;x6D;x51;x5A;x77;x42;x53;x4D;x41;x63;x41;x55;x6A;x6D;x54;x46;x34;x4A;x56;x55;x51;x65;x43;x56;x56;x47;x54;x46;x67;x57;x59;x49;x41;x59;x46;x6D;x43;x6D;x54;x46;x34;x4A;x5A;x6B;x41;x65;x43;x57;x5A;x6D;x54;x46;x34;x4A;x67;x45;x41;x65;x43;x59;x42;x6D;x54;x46;x77;x45;x30;x49;x41;x63;x42;x4E;x43;x43;x54;x46;x77;x68;x43;x63;x52;x63;x49;x51;x6E;x45;x54;x46;x34;x4A;x55;x59;x41;x65;x43;x56;x47;x6D;x54;x43;x42;x41;x41;x41;x41;x67;x5A;x6D;x5A;x6D;x51;x36;x51;x41;x41;x41;x41;x6B;x46;x43;x5A;x6D;x51;x36;x51;x55;x51;x41;x41;x6B;x46;x47;x5A;x6D;x52;x4B;x51;x55;x67;x41;x41;x6B;x4A;x6D;x5A;x6D;x51;x36;x52;x45;x51;x41;x41;x6B;x52;x47;x5A;x6D;x51;x36;x55;x41;x41;x41;x41;x6D;x59;x6D;x5A;x6D;x51;x36;x5A;x6B;x41;x41;x41;x6D;x5A;x6D;x5A;x6D;x51;x6A;x2F;x42;x6F;x49;x42;x67;x39;x38;x4E;x41;x51;x4C;x66;x46;x6F;x49;x42;x65;x69;x49;x43;x49;x41;x41;x69;x41;x69;x43;x5A;x2F;x7A;x64;x52;x49;x67;x41;x33;x55;x53;x53;x5A;x2F;x30;x42;x44;x4A;x41;x42;x41;x51;x79;x53;x5A;x2F;x6B;x43;x41;x63;x77;x42;x41;x67;x48;x53;x5A;x2F;x6B;x43;x43;x51;x41;x42;x41;x67;x6B;x47;x5A;x2F;x6B;x43;x44;x52;x41;x42;x41;x67;x30;x57;x5A;x2F;x6B;x43;x57;x55;x77;x42;x41;x6C;x6C;x4F;x5A;x2F;x6B;x45;x44;x51;x41;x42;x4B;x63;x77;x3D;x3D;
//2849=>712
x03; // Control_ETX

BASE64(ABAACQIiAACAAhkABAIHBABTAEJHUwFNUasARONQAgAAAB4AaQAAAAIBNDxHAAIJtQAA/ngAAAAA/waCAYPfDQEB3xaCAXoiAAAAIgSZmRIiIQAAJyCZmQEwAAAAMJmZmQYwiAAAMJSZmRUwlgAAMQKZmRUxEgAAMSCZmRUxWAAAMVmZmRUzNwAAM0mZmRU0AAAANJmZmQU1KAAANYmZmRU2AAAANpmZmQY3AAAAN5mZmQU4AAAAOZmZmQZAAAAASZmZmQNCdoAAQnaJmQRQAAAAUJmZmQJRAAAAVZmZmQFWAAAAWZmZmQJgAAAAaZmZmQJgEQAAYBEJmQZgESAAYBFJmQZgEXQAYBF0mQZgEXcAYBF3mQZgEYYAYBGZmQZgVGEAYFRimQdiAAAAYpmZmQ5kQAAAZZmZmQZwBSMAcAUjmTF4JVUQeCVVGTFgWYIAYFmCmTF4JZkAeCWZmTF4JgEAeCYBmTFwE0IAcBNCCTFwhCcRcIQnETF4JUYAeCVGmTCBAAAAgZmZmQ6QAAAAkFCZmQ6QUQAAkFGZmRKQUgAAkJmZmQ6REQAAkRGZmQ6UAAAAmYmZmQ6ZkAAAmZmZmQj/BoIBg98NAQLfFoIBeiICIAAiAiCZ/zdRIgA3USSZ/0BDJABAQySZ/kCAcwBAgHSZ/kCCQABAgkGZ/kCDRABAg0WZ/kCWUwBAllOZ/kEDQABKcw==)

00 // orderNum=0
10 // len=wrong
00 09 02 22 00 00 80 02 19 00 04 02 07 04 00 53 00 42 47 53 01 4d 51 ab 00 44 e3 50 02 00 00 00 1e 00 69 00 00 00 02 01 34 3c 47 00 02 09 b5 00 00 fe 78 00 00 00 00 ff 06 82 01 83 df 0d 01 01 df 16 82 01 7a 22 00 00 00 22 04 99 99 12 22 21 00 00 27 20 99 99 01 30 00 00 00 30 99 99 99 06 30 88 00 00 30 94 99 99 15 30 96 00 00 31 02 99 99 15 31 12 00 00 31 20 99 99 15 31 58 00 00 31 59 99 99 15 33 37 00 00 33 49 99 99 15 34 00 00 00 34 99 99 99 05 35 28 00 00 35 89 99 99 15 36 00 00 00 36 99 99 99 06 37 00 00 00 37 99 99 99 05 38 00 00 00 39 99 99 99 06 40 00 00 00 49 99 99 99 03 42 76 80 00 42 76 89 99 04 50 00 00 00 50 99 99 99 02 51 00 00 00 55 99 99 99 01 56 00 00 00 59 99 99 99 02 60 00 00 00 69 99 99 99 02 60 11 00 00 60 11 09 99 06 60 11 20 00 60 11 49 99 06 60 11 74 00 60 11 74 99 06 60 11 77 00 60 11 77 99 06 60 11 86 00 60 11 99 99 06 60 54 61 00 60 54 62 99 07 62 00 00 00 62 99 99 99 0e 64 40 00 00 65 99 99 99 06 70 05 23 00 70 05 23 99 31 78 25 55 10 78 25 55 19 31 60 59 82 00 60 59 82 99 31 78 25 99 00 78 25 99 99 31 78 26 01 00 78 26 01 99 31 70 13 42 00 70 13 42 09 31 70 84 27 11 70 84 27 11 31 78 25 46 00 78 25 46 99 30 81 00 00 00 81 99 99 99 0e 90 00 00 00 90 50 99 99 0e 90 51 00 00 90 51 99 99 12 90 52 00 00 90 99 99 99 0e 91 11 00 00 91 11 99 99 0e 94 00 00 00 99 89 99 99 0e 99 90 00 00 99 99 99 99 08 ff 06 82 01 83 df 0d 01 02 df 16 82 01 7a 22 02 20 00 22 02 20 99 ff 37 51 22 00 37 51 24 99 ff 40 43 24 00 40 43 24 99 fe 40 80 73 00 40 80 74 99 fe 40 82 40 00 40 82 41 99 fe 40 83 44 00 40 83 45 99 fe 40 96 53 00 40 96 53 99 fe 41 03 40 00
4a 73 // crc
 */
bool FrameLayer::sendControl(uint8_t byte) {
	LOG_DEBUG(LOG_ECLF, "sendControl");
	uart->send(byte);
	return true;
}

void FrameLayer::sendData(uint8_t *data, uint16_t dataLen) {
	for(uint16_t i = 0; i < dataLen; i++) {
		uart->send(data[i]);
	}
}

void FrameLayer::handle() {
	while(uart->isEmptyReceiveBuffer() == false) {
		LOG_TRACE(LOG_ECLF, "have data " << state);
		switch(state) {
		case State_STX: stateStxRecv(); break;
		case State_MARK: stateMarkRecv(); break;
		case State_ETX: stateEtxRecv(); break;
		default: LOG_ERROR(LOG_ECLF, "Unwaited data state=" << state << ", byte=" << uart->receive());
		}
	}
}

void FrameLayer::procTimer() {
	LOG_DEBUG(LOG_ECLF, "procTimer " << state);
	switch(state) {
	default: LOG_ERROR(LOG_ECLF, "Unwaited timeout state=" << state);
	}
}

void FrameLayer::gotoStateStx() {
	LOG_DEBUG(LOG_ECLF, "gotoStateStx");
//	timer->stop();
	recvBuf.clear();
	state = State_STX;
}

void FrameLayer::stateStxRecv() {
	LOG_DEBUG(LOG_ECLF, "stateStxRecv");
	uint8_t b1 = uart->receive();
	LOG_TRACE_HEX(LOG_ECLF, b1);
	if(b1 == Control_STX) {
		gotoStateMark();
		return;
	} else if(b1 == Control_EOT) {
		observer->procControl(b1);
		return;
	} else if(b1 == Control_ACK) {
		observer->procControl(b1);
		return;
	} else if(b1 == Control_BEL) {
		observer->procControl(b1);
		return;
	} else if(b1 == Control_NAK) {
		LOG_ERROR(LOG_ECLF, "Receive NAK " << sendBuf.getLen());
#if 1
//+++
		uart->send(Control_STX);
		uart->send(Control_MARK);
		sendData(sendBuf.getData(), sendBuf.getLen());
		uart->send(Control_ETX);
		LOG(">>>>>>>>>" << sendBuf.getLen() << "/" << sendBuf.getSize());
//+++
#endif
		return;
	} else {
		LOG_ERROR(LOG_ECLF, "Unwaited answer " << b1);
		return;
	}
}

void FrameLayer::gotoStateMark() {
	LOG_INFO(LOG_ECLF, "gotoStateMark");
//	timer->start(SBERBANK_RECV_TIMEOUT);
	recvBuf.clear();
	state = State_MARK;
}

void FrameLayer::stateMarkRecv() {
	LOG_INFO(LOG_ECLF, "stateMarkRecv");
	uint8_t b1 = uart->receive();
	LOG_TRACE_HEX(LOG_ECLF, b1);
	if(b1 != Control_MARK) {
		gotoStateStx();
		return;
	}
	gotoStateEtx();
}

void FrameLayer::gotoStateEtx() {
	LOG_INFO(LOG_ECLF, "gotoStateEtx");
	recvBuf.clear();
	state = State_ETX;
}

void FrameLayer::stateEtxRecv() {
	LOG_DEBUG(LOG_ECLF, "stateEtxRecv");
	uint8_t b1 = uart->receive();
	LOG_TRACE_HEX(LOG_ECLF, b1);
	if(b1 != Control_ETX) {
		recvBuf.addUint8(b1);
		return;
	}

	LOG_INFO(LOG_ECLF, "received " << recvBuf.getLen());
	LOG_INFO_HEX(LOG_ECLF, recvBuf.getData(), recvBuf.getLen());
	uint16_t dataLen = convertBase64ToBin(recvBuf.getData(), recvBuf.getLen(), recvBuf.getData(), recvBuf.getSize());
	if(dataLen == 0) {
		gotoStateStx();
		return;
	}

	gotoStateStx();
	observer->procPacket(recvBuf.getData(), dataLen);
}

}
