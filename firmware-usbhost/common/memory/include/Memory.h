#ifndef COMMON_MEMORY_MEMORY_H_
#define COMMON_MEMORY_MEMORY_H_

#include <stdint.h>

enum MemoryResult {
	MemoryResult_Ok = 0,
	MemoryResult_WrongCrc = 1,
	MemoryResult_ReadError = 2,
	MemoryResult_WriteError = 3,
	MemoryResult_WrongVersion = 4,
	MemoryResult_OutOfIndex = 5,
	MemoryResult_NotFound = 6,
	MemoryResult_WrongData = 7,
	MemoryResult_ProgError = 8,
};

class Memory {
public:
	Memory() : address(0) {}
	virtual ~Memory() {}

	virtual void setAddress(uint32_t address) { this->address = address; }
	virtual uint32_t getAddress() { return address; }
	virtual void skip(uint32_t len) { address += len; }
	virtual MemoryResult write(const void *pData, const uint32_t len) = 0;
	virtual MemoryResult read(void *pData, const uint32_t len) = 0;
	virtual uint32_t getMaxSize() const = 0;
	virtual uint32_t getPageSize() const = 0;

protected:
	uint32_t address;
};

#endif
