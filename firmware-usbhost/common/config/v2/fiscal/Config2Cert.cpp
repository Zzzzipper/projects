#include "Config2Cert.h"

#include "utils/include/NetworkProtocol.h"
#include "memory/include/Memory.h"
#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#pragma pack(push,1)
struct Config2CertStruct {
	uint16_t size;
};
#pragma pack(pop)

Config2Cert::Config2Cert() :
	memory(NULL)
{
}

MemoryResult Config2Cert::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	StringBuilder data;
	return save(&data);
}

MemoryResult Config2Cert::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	MemoryCrc crc(memory);
	crc.startCrc();

	Config2CertStruct header;
	MemoryResult result = crc.read((uint8_t*)&header, sizeof(header));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Read hdr failed");
		return result;
	}
	if(header.size > CERT_SIZE) {
		LOG_ERROR(LOG_CFG, "Cert size too big " << header.size);
		return MemoryResult_WrongData;
	}

	result = loadPadding(&crc, CERT_SIZE);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Padding read failed");
		return result;
	}

	result = crc.readCrc();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Cert crc failed");
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config2Cert::load(StringBuilder *buf) {
	memory->setAddress(address);
	MemoryCrc crc(memory);
	crc.startCrc();

	Config2CertStruct header;
	MemoryResult result = crc.read((uint8_t*)&header, sizeof(header));
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Header read failed");
		return result;
	}
	if(header.size > CERT_SIZE || header.size > buf->getSize()) {
		LOG_ERROR(LOG_CFG, "Cert size too big " << header.size);
		return MemoryResult_WrongData;
	}

	result = crc.read(buf->getData(), header.size);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Cert read failed");
		return result;
	}
	buf->setLen(header.size);

	uint16_t paddingSize = CERT_SIZE - header.size;
	result = loadPadding(&crc, paddingSize);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Padding read failed");
		return result;
	}

	result = crc.readCrc();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Cert crc failed");
		return result;
	}

	return MemoryResult_Ok;
}

MemoryResult Config2Cert::loadPadding(MemoryCrc *crc, uint16_t size) {
	uint8_t buf[128];
	uint16_t len = sizeof(buf);
	for(uint16_t i = 0; i < size; i += len) {
		uint16_t tailLen = size - i;
		if(len > tailLen) { len = tailLen; }
		MemoryResult result = crc->read(buf, len);
		if(result != MemoryResult_Ok) { return result; }
	}
	return MemoryResult_Ok;
}

MemoryResult Config2Cert::save(StringBuilder *cert) {
	memory->setAddress(address);
	MemoryCrc crc(memory);
	crc.startCrc();

	Config2CertStruct header;
	header.size = cert->getLen();
	MemoryResult result = crc.write((uint8_t*)&header, sizeof(header));
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = crc.write(cert->getData(), cert->getLen());
	if(result != MemoryResult_Ok) {
		return result;
	}

	uint16_t paddingSize = CERT_SIZE - cert->getLen();
	uint8_t paddingData[128];
	for(uint16_t i = 0; i < sizeof(paddingData); i++) {
		paddingData[i] = 0;
	}

	uint16_t paddingLen = sizeof(paddingData);
	for(uint16_t i = 0; i < paddingSize; i += paddingLen) {
		uint16_t tailLen = paddingSize - i;
		if(paddingLen > tailLen) { paddingLen = tailLen; }
		result = crc.write(paddingData, paddingLen);
		if(result != MemoryResult_Ok) {
			return result;
		}
	}

	return crc.writeCrc();
}
