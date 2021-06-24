#ifndef COMMON_CONFIG_V2_CERT_H_
#define COMMON_CONFIG_V2_CERT_H_

#include "memory/include/Memory.h"
#include "utils/include/StringBuilder.h"

#define CERT_SIZE 2048

class Memory;
class MemoryCrc;

class Config2Cert {
public:
	Config2Cert();
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult load(StringBuilder *buf);
	MemoryResult save(StringBuilder *data);

private:
	Memory *memory;
	uint32_t address;

	MemoryResult loadPadding(MemoryCrc *crc, uint16_t size);
};

#endif
