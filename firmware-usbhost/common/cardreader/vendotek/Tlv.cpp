#include "Tlv.h"

#include "logger/include/Logger.h"

#include <string.h>

namespace Tlv {

bool Header::parse(uint32_t dataLen, uint32_t *headerLen, uint32_t *valueLen) {
	uint32_t hl = sizeof(Header);
	uint32_t vl = 0;

	if(dataLen < hl) { LOG_DEBUG(LOG_JSON, "Error"); return false; }

	if(this->len < 0x81) {
		vl = this->len;
	} else if(this->len == 0x81) {
		hl += 1;
		if(dataLen < hl) { LOG_DEBUG(LOG_JSON, "Error"); return false; }
		vl = this->value[0];
	} else if(this->len == 0x82) {
		hl += 2;
		if(dataLen < hl) { LOG_DEBUG(LOG_JSON, "Error"); return false; }
		LEUint2 *n = (LEUint2*)(this->value);
		vl = n->get();
	} else if(this->len == 0x83) {
		hl += 3;
		if(dataLen < hl) { LOG_DEBUG(LOG_JSON, "Error"); return false; }
		LEUint3 *n = (LEUint3*)(this->value);
		vl = n->get();
	} else if(this->len == 0x84) {
		hl += 4;
		if(dataLen < hl) { LOG_DEBUG(LOG_JSON, "Error"); return false; }
		LEUint4 *n = (LEUint4*)(this->value);
		vl = n->get();
	} else {
		LOG_DEBUG(LOG_JSON, "Error");
		return false;
	}

	if((hl + vl) > dataLen) { LOG_DEBUG(LOG_JSON, "Error"); return false; }
	if(headerLen != NULL) { *headerLen = hl; }
	if(valueLen != NULL) { *valueLen = vl; }
	return true;
}

uint8_t *Header::getValue() {
	return (this->value + getValueOffset());
}

uint32_t Header::getValueLen() {
	if(this->len < 0x81) {
		return this->len;
	} else if(this->len == 0x81) {
		return this->value[0];
	} else if(this->len == 0x82) {
		LEUint2 *n = (LEUint2*)(this->value);
		return n->get();
	} else if(this->len == 0x83) {
		LEUint3 *n = (LEUint3*)(this->value);
		return n->get();
	} else if(this->len == 0x84) {
		LEUint4 *n = (LEUint4*)(this->value);
		return n->get();
	} else {
		return 0;
	}
}

uint32_t Header::getValueOffset() {
	if(this->len < 0x81) {
		return 0;
	} else if(this->len == 0x81) {
		return 1;
	} else if(this->len == 0x82) {
		return 2;
	} else if(this->len == 0x83) {
		return 3;
	} else if(this->len == 0x84) {
		return 4;
	} else {
		return 0;
	}
}

Packet::Packet(uint16_t nodeMaxNumber) :
	nodeNum(0),
	nodeMax(nodeMaxNumber)
{
	nodes = new Header*[nodeMax];
}

Packet::~Packet() {
	delete nodes;
}

bool Packet::parse(const uint8_t *data, uint32_t dataLen) {
	const uint8_t *unparsed = data;
	uint32_t unparsedLen = dataLen;
	uint32_t headerLen;
	uint32_t valueLen;
	for(nodeNum = 0; nodeNum < nodeMax;) {
		Header *h = (Header*)unparsed;
		if(h->parse(unparsedLen, &headerLen, &valueLen) == false) {
			LOG_ERROR(LOG_JSON, "Bad format " << nodeNum << "," << unparsedLen << "/" << dataLen);
			return false;
		}

		nodes[nodeNum] = h;
		nodeNum++;
		unparsed += (headerLen + valueLen);
		unparsedLen -= (headerLen + valueLen);
		LOG_DEBUG(LOG_JSON, "add " << nodeNum << "," << headerLen << "*" << valueLen << "," << unparsedLen << "/" << dataLen);
		if(unparsedLen == 0) {
			return true;
		}
	}

	LOG_ERROR(LOG_JSON, "Too many nodes");
	return false;
}

bool Packet::getNumber(uint8_t id, uint16_t *num) {
	Header *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->getValue();
	uint16_t len = h->getValueLen();
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

bool Packet::getNumber(uint8_t id, uint32_t *num) {
	Header *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->getValue();
	uint16_t len = h->getValueLen();
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

bool Packet::getNumber(uint8_t id, uint64_t *num) {
	Header *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->getValue();
	uint16_t len = h->getValueLen();
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

bool Packet::getString(uint8_t id, StringBuilder *str) {
	Header *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->getValue();
	uint16_t len = h->getValueLen();
	str->clear();
	for(uint16_t i = 0; i < len; i++) {
		str->add(d[i]);
	}

	return true;
}

uint32_t Packet::getData(uint8_t id, void *buf, uint32_t bufSize) {
	Header *h = find(id);
	if(h == NULL) {
		return 0;
	}

	uint8_t *d = h->getValue();
	uint16_t len = h->getValueLen();
	uint8_t *b = (uint8_t*)buf;
	uint16_t i = 0;
	for(; i < len && i < bufSize; i++) {
		b[i] = d[i];
	}

	return i;
}

bool Packet::getDateTime(uint8_t id, DateTime *date) {
	Header *h = find(id);
	if(h == NULL) {
		return false;
	}

	uint8_t *d = h->getValue();
	if(h->getValueLen() != 14) {
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

Header *Packet::find(uint8_t id) {
	for(uint16_t i = 0; i < nodeNum; i++) {
		if(nodes[i]->type == id) {
			return nodes[i];
		}
	}
	return NULL;
}

void Packet::print() {
	for(uint16_t i = 0; i < nodeNum; i++) {
		LOG("type=" << nodes[i]->type << ",len=" << nodes[i]->len << ",value=");
		LOG_HEX(nodes[i]->value, nodes[i]->len);
	}
}

PacketMaker::PacketMaker(uint16_t bufSize) : buf(bufSize) {

}

Buffer *PacketMaker::getBuf() {
	return &buf;
}

const uint8_t *PacketMaker::getData() {
	return buf.getData();
}

uint16_t PacketMaker::getDataLen() {
	return buf.getLen();
}

void PacketMaker::addLength(uint32_t len) {
	if(len < 0x81) {
		buf.addUint8(len);
		return;
	} else if(len <= 0xFF) {
		buf.addUint8(0x81);
		buf.addUint8(len);
		return;
	} else if(len <= 0xFFFF) {
		buf.addUint8(0x82);
		LEUint2 *n = (LEUint2*)(buf.getData() + buf.getLen());
		n->set(len);
		buf.setLen(buf.getLen() + 2);
		return;
	} else if(len <= 0xFFFFFF) {
		buf.addUint8(0x83);
		LEUint3 *n = (LEUint3*)(buf.getData() + buf.getLen());
		n->set(len);
		buf.setLen(buf.getLen() + 3);
		return;
	} else if(len <= 0xFFFFFFFF) {
		buf.addUint8(0x84);
		LEUint4 *n = (LEUint4*)(buf.getData() + buf.getLen());
		n->set(len);
		buf.setLen(buf.getLen() + 4);
		return;
	}
}

bool PacketMaker::addNumber(uint8_t id, uint32_t num) {
	uint16_t l = 0;
	uint32_t n = num;
	do {
		l++;
		n = n / 10;
	} while(n > 0);
	return addNumber(id, l, num);
}

bool PacketMaker::addNumber(uint8_t id, uint16_t maxSize, uint32_t num) {
	if((buf.getSize() - buf.getLen()) < (sizeof(Header) + maxSize)) {
		return false;
	}
	buf.addUint8(id);
	addLength(maxSize);

	uint8_t *d = buf.getData() + buf.getLen();
	uint32_t n = num;
	for(uint16_t i = maxSize; i > 0;) {
		i--;
		d[i] = 0x30 + n % 10;
		n = n / 10;
	}

	buf.setLen(buf.getLen() + maxSize);
	return true;
}

bool PacketMaker::addString(uint8_t id, const char *str) {
	uint32_t strLen = strlen(str);
	return addString(id, str, strLen);
}

bool PacketMaker::addString(uint8_t id, const char *str, uint16_t strLen) {
	if((buf.getSize() - buf.getLen()) < (sizeof(Header) + strLen)) {
		return false;
	}
	buf.addUint8(id);
	addLength(strLen);
	buf.add(str, strLen);
	return true;
}

bool PacketMaker::addData(uint8_t id, const void *data, uint16_t dataLen) {
	if((buf.getSize() - buf.getLen()) < (sizeof(Header) + dataLen)) {
		return false;
	}
	buf.addUint8(id);
	addLength(dataLen);
	buf.add(data, dataLen);
	return true;
}

//20200316T160242+0300
bool PacketMaker::addDateTime(uint8_t id, DateTime *datetime) {
	int32_t totalSeconds = datetime->getTotalSeconds();
	uint8_t timeZone = 3;
	totalSeconds -= timeZone*3600;
	DateTime d;
	d.set(totalSeconds);

	buf.addUint8(id);
	addLength(20);
	if(d.year >= 70) {
		buf.addUint8('1');
		buf.addUint8('9');
	} else {
		buf.addUint8('2');
		buf.addUint8('0');
	}
	uint8_t hyear = d.year / 10;
	uint8_t lyear = d.year % 10;
	buf.addUint8(0x30 + hyear);
	buf.addUint8(0x30 + lyear);
	uint8_t hmonth = d.month / 10;
	uint8_t lmonth = d.month % 10;
	buf.addUint8(0x30 + hmonth);
	buf.addUint8(0x30 + lmonth);
	uint8_t hday = d.day / 10;
	uint8_t lday = d.day % 10;
	buf.addUint8(0x30 + hday);
	buf.addUint8(0x30 + lday);

	buf.addUint8('T');
	uint8_t hhour = d.hour / 10;
	uint8_t lhour = d.hour % 10;
	buf.addUint8(0x30 + hhour);
	buf.addUint8(0x30 + lhour);
	uint8_t hminute = d.minute / 10;
	uint8_t lminute = d.minute % 10;
	buf.addUint8(0x30 + hminute);
	buf.addUint8(0x30 + lminute);
	uint8_t hsecond = d.second / 10;
	uint8_t lsecond = d.second % 10;
	buf.addUint8(0x30 + hsecond);
	buf.addUint8(0x30 + lsecond);

	buf.addUint8('+');
	buf.addUint8('0');
	buf.addUint8(0x30 + timeZone);
	buf.addUint8('0');
	buf.addUint8('0');
	return true;
}

void PacketMaker::clear() {
	buf.clear();
}

}
