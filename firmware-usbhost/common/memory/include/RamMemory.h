#ifndef COMMON_MEMORY_RAMMEMORY_H_
#define COMMON_MEMORY_RAMMEMORY_H_

#include "memory/include/Memory.h"
#include "utils/include/Buffer.h"

class RamMemory : public Memory {
public:
	RamMemory(uint32_t bufSize) : buf(bufSize) { address = 0; }
	virtual MemoryResult write(const void *pData, uint32_t len) override;
	virtual MemoryResult read(void *pData, uint32_t size) override;
	virtual uint32_t getMaxSize() const override;
	virtual uint32_t getPageSize() const override;
	uint8_t *getData() { return buf.getData(); }
	void clear();
	void fill(uint8_t b);
	void printAllData();

protected:
	Buffer buf;
};

#endif
