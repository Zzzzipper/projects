#include "InpasProtocol.h"

#include "logger/include/Logger.h"

namespace Inpas {

TlvPacket::TlvPacket(uint16_t nodeMaxNumber) :
	nodeNum(0),
	nodeMax(nodeMaxNumber)
{
	nodes = new TlvHeader*[nodeMax];
}

TlvPacket::~TlvPacket() {
	delete nodes;
}

bool TlvPacket::parse(const uint8_t *data, uint16_t dataLen) {
	uint16_t dataCount = 0;
	uint16_t tlvHeaderSize = sizeof(TlvHeader);
	for(nodeNum = 0; nodeNum < nodeMax;) {
		TlvHeader *h = (TlvHeader*)(data + dataCount);
		if((dataLen - dataCount) < tlvHeaderSize) {
			LOG_ERROR(LOG_JSON, "Bad format " << nodeNum);
			return false;
		}

		dataCount += tlvHeaderSize;
		if((dataLen - dataCount) < h->len.get()) {
			LOG_ERROR(LOG_JSON, "Bad format " << (dataLen - dataCount) << "," << h->len.get());
			return false;
		}

		nodes[nodeNum] = h;
		nodeNum++;
		dataCount += h->len.get();
		if(dataCount == dataLen) {
			return true;
		}
	}

	LOG_ERROR(LOG_JSON, "Too many nodes");
	return false;
}

bool TlvPacket::getNumber(uint8_t id, uint16_t *num) {
	TlvHeader *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->data;
	uint16_t len = h->len.get();
	uint16_t n = 0;
	for(uint16_t i = 0; i < len; i++) {
		if(d[i] < 0x30 || d[i] > 0x39) {
			return false;
		}
		n = n * 10 + (d[i] - 0x30);
	}

	*num = n;
	return true;
}

bool TlvPacket::getNumber(uint8_t id, uint32_t *num) {
	TlvHeader *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->data;
	uint16_t len = h->len.get();
	uint32_t n = 0;
	for(uint16_t i = 0; i < len; i++) {
		if(d[i] < 0x30 || d[i] > 0x39) {
			return false;
		}
		n = n * 10 + (d[i] - 0x30);
	}

	*num = n;
	return true;
}

bool TlvPacket::getNumber(uint8_t id, uint64_t *num) {
	TlvHeader *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->data;
	uint16_t len = h->len.get();
	uint64_t n = 0;
	for(uint16_t i = 0; i < len; i++) {
		if(d[i] < 0x30 || d[i] > 0x39) {
			return false;
		}
		n = n * 10 + (d[i] - 0x30);
	}

	*num = n;
	return true;
}

bool TlvPacket::getString(uint8_t id, StringBuilder *str) {
	TlvHeader *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->data;
	uint16_t len = h->len.get();
	str->clear();
	for(uint16_t i = 0; i < len; i++) {
		str->add(d[i]);
	}

	return true;
}

bool TlvPacket::getDateTime(uint8_t id, DateTime *date) {
	TlvHeader *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->data;
	if(h->len.get() != 14) {
		return false;
	}

	uint16_t i = 0;
	uint16_t year = 0;
	for(; i < 4; i++) {
		if(d[i] < 0x30 || d[i] > 0x39) {
			return false;
		}
		year = year * 10 + (d[i] - 0x30);
	}
	date->year = year - 2000;
	date->month = 0;
	for(; i < 6; i++) {
		if(d[i] < 0x30 || d[i] > 0x39) {
			return false;
		}
		date->month = date->month * 10 + (d[i] - 0x30);
	}
	date->day = 0;
	for(; i < 8; i++) {
		if(d[i] < 0x30 || d[i] > 0x39) {
			return false;
		}
		date->day = date->day * 10 + (d[i] - 0x30);
	}

	date->hour = 0;
	for(; i < 10; i++) {
		if(d[i] < 0x30 || d[i] > 0x39) {
			return false;
		}
		date->hour = date->hour * 10 + (d[i] - 0x30);
	}
	date->minute = 0;
	for(; i < 12; i++) {
		if(d[i] < 0x30 || d[i] > 0x39) {
			return false;
		}
		date->minute = date->minute * 10 + (d[i] - 0x30);
	}
	date->second = 0;
	for(; i < 14; i++) {
		if(d[i] < 0x30 || d[i] > 0x39) {
			return false;
		}
		date->second = date->second * 10 + (d[i] - 0x30);
	}

	return true;
}

bool TlvPacket::getMdbOptions(uint8_t id, uint32_t *decimalPoint, uint32_t *scaleFactor, uint8_t *version) {
	TlvHeader *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->data;
	if(h->len.get() != 6) {
		return false;
	}

	for(uint16_t i = 0; i < 6; i++) {
		if(d[i] < 0x30 || d[i] > 0x39) {
			return false;
		}
	}

	*decimalPoint = (d[0] - 0x30) + (d[1] - 0x30) * 10;
	*scaleFactor = (d[2] - 0x30) + (d[3] - 0x30) * 10;
	*version = (d[4] - 0x30) + (d[5] - 0x30) * 10;
	return true;
}

TlvHeader *TlvPacket::find(uint8_t id) {
	for(uint16_t i = 0; i < nodeNum; i++) {
		if(nodes[i]->type == id) {
			return nodes[i];
		}
	}
	return NULL;
}

TlvPacketMaker::TlvPacketMaker(uint16_t bufSize) : buf(bufSize) {

}

Buffer *TlvPacketMaker::getBuf() {
	return &buf;
}

const uint8_t *TlvPacketMaker::getData() {
	return buf.getData();
}

uint16_t TlvPacketMaker::getDataLen() {
	return buf.getLen();
}

bool TlvPacketMaker::addNumber(uint8_t id, uint16_t dataSize, uint32_t num) {
	if((buf.getSize() - buf.getLen()) < (sizeof(TlvHeader) + dataSize)) {
		return false;
	}
	buf.addUint8(id);
	buf.addUint8(dataSize);
	buf.addUint8(dataSize >> 8);

	uint8_t *d = buf.getData() + buf.getLen();
	uint32_t n = num;
	for(uint16_t i = dataSize; i > 0;) {
		i--;
		d[i] = 0x30 + n % 10;
		n = n / 10;
	}

	buf.setLen(buf.getLen() + dataSize);
	return true;
}

bool TlvPacketMaker::addString(uint8_t id, const char *str, uint16_t strLen) {
	if((buf.getSize() - buf.getLen()) < (sizeof(TlvHeader) + strLen)) {
		return false;
	}
	buf.addUint8(id);
	buf.addUint8(strLen);
	buf.addUint8(strLen >> 8);
	buf.add(str, strLen);
	return true;
}

void TlvPacketMaker::clear() {
	buf.clear();
}

}

/*
19.02.25 15:27:50 <02<17<00<00<05<00<32<30<30<30<30<04<03<00<36<34<33<56<06<00<32<30<31<30<30<30<90<F9

19.02.25 15:27:56 >06

19.02.25 15:28:02 <04<02<17<00<00<05<00<32<30<30<30<30<04<03<00<36<34<33<56<06<00<32<30<31<30<30<30<90<F9

19.02.25 15:28:06 >06

19.02.25 15:28:25 <04<02<17<00<00<05<00<32<30<30<30<30<04<03<00<36<34<33<56<06<00<32<30<31<30<30<30<90<F9

19.02.25 15:29:15 >06

19.02.25 15:29:45 <04<02<17<00<00<05<00<32<30<30<30<30<04<03<00<36<34<33<56<06<00<32<30<31<30<30<30<90<F9

19.02.25 15:29:52 >06>02>15>00>00>08>00>30>30>30>30>31>30>30>30>04>03>00>36>34>33>19>01>00>31>38>AF

19.02.25 15:30:10 <06<02<05<00<19<02<00<32<31<A5<C2

19.02.25 15:30:17 >06

19.02.25 15:30:17 <02<9A<00<00<04<00<31<30<30<30<04<03<00<36<34<33<06<0E<00<32<30<31<39<30<32<32<35<31<35<33<32<30<32<0A<10<00<2A<2A<2A<2A<2A<2A<2A<2A<2A<2A<2A<2A<36<36<33<39<0B<04<00<31<39<30<33<0D<06<00<32<31<32<38<34<30<0E<0C<00<39<30<35<36<33<32<36<32<35<35<36<38<0F<02<00<30<30<13<08<00<CE<C4<CE<C1<D0<C5<CD<CE<15<0E<00<32<30<31<39<30<32<32<35<31<35<33<32<30<32<17<02<00<2D<31<19<01<00<31<1A<02<00<2D<31<1B<08<00<30<30<32<36<36<32<38<36<1C<09<00<31<31<31<31<31<31<31<31<31<27<01<00<31<34<FE

19.02.25 15:31:31 >06>04>

02>10>00>19>02>00>35>33>00>08>00>30>30>30>30>31>30>30>30>30>09
STX(x02;)LEN(x10;x00;=16)
FIELD(x19;=operation_code)LEN(x02;x00;)VALUE(x35;x33=53)
FIELD(x00;=credit)LEN(x08;x00;)VALUE(x30;x30;x30;x30;x30;x31;x30;x30;=00000100)
CRC(x30;x09;)

// автоотмена
19.02.25 15:31:40 <06

// transaction wait
<x02;x05;x00;x19; x02;x00;x32;x31;xA5; xC2;
STX(x02;)LEN(x05;x00;=5)
FIELD(x19;=)LEN(x02;x00;)VALUE(x32;x31;=wait)
CRC(xA5;xC2;)


19.02.25 15:31:48 >06

19.02.25 15:31:48 <02<7A<00<00<04<00<31<30<30<30<04<01<00<30<06<0E<00<32<30<31<39<30<32<32<35<31<35<33<33<35<31<0F<03<00<45<52<33<13<21<00<CE<CF<C5<D0<C0<D6<C8<DF<20<CF<D0<C5<D0<C2<C0<CD<C0<5E<54<45<52<4D<49<4E<41<54<45<44<2E<4A<50<47<7E<15<0E<00<32<30<31<39<30<32<32<35<31<35<33<33<35<31<17<02<00<2D<31<19<02<00<35<33<1A<02<00<2D<31<1B<08<00<30<30<32<36<36<32<38<36<1C<01<00<30<27<02<00<35<33<10<5D

19.02.25 15:31:57 >06

19.02.25 15:31:57 <02<17<00<00<05<00<32<30<30<30<30<04<03<00<36<34<33<56<06<00<32<30<31<30<30<30<90<F9

19.02.25 15:32:01
 */
