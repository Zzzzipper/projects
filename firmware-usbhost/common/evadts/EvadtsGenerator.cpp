#include "EvadtsGenerator.h"
#include "dex/DexProtocol.h"
#include "utils/include/StringBuilder.h"
#include "logger/include/Logger.h"

EvadtsGenerator::EvadtsGenerator(const char *communicactionId) :
	communicactionId(communicactionId)
{
	this->str = new StringBuilder(DEX_DATA_MAX_SIZE, DEX_DATA_MAX_SIZE);
}

EvadtsGenerator::~EvadtsGenerator() {
	delete this->str;
}

const void *EvadtsGenerator::getData() {
	return str->getString();
}

uint16_t EvadtsGenerator::getLen() {
	return str->getLen();
}

void EvadtsGenerator::startBlock() {
	str->clear();
}

void EvadtsGenerator::win1251ToLatin(const char *string) {
	const char a[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	uint8_t *s = (uint8_t*)string;
	while(*s != '\0') {
		if(*s >= 0x80 || *s == '*' || *s == '^') {
			str->add('^');
			str->add(a[*s >> 4]);
			str->add(a[*s & 0x0F]);
			str->add('^');
		} else {
			str->add(*s);
		}
		s++;
	}
}

void EvadtsGenerator::finishLine() {
	*str << "\r\n";
	strCount++;
}

void EvadtsGenerator::finishBlock() {
	LOG_DEBUG_STR(LOG_EVADTS, str->getString(), str->getLen());
	crc.add(str->getString());
	if(str->getLen() >= DEX_DATA_MAX_SIZE) {
		LOG_ERROR(LOG_EVADTS, "Buffer overflow: " << str->getLen() << "/" << str->getSize());
	}
}

/*
DXS*XYZ1234567*VA*V0/6*1
ST*001*0001
...
G85*1234 (example G85 CRC is 1234)
SE*9*0001
DXE*1*1
*/
void EvadtsGenerator::generateHeader() {
	str->clear();
	*str << "DXS*" << communicactionId << "*VA*V0/6*1\r\n";
	*str << "ST*001*0001\r\n";

	strCount = 1; // start with and including the ST

	crc.start();
	crc.add("ST*001*0001\r\n");
}

void EvadtsGenerator::generateFooter() {
	strCount += 2; // finishing with and including the SE

	str->clear();
	*str << "G85*";	str->addHex(crc.getHighByte()); str->addHex(crc.getLowByte()); *str << "\r\n";
	*str << "SE*" << strCount << "*0001\r\n";
	*str << "DXE*1*1\r\n";
}
