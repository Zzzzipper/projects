#ifndef COMMON_MEMORY_FILEMEMORY_H_
#define COMMON_MEMORY_FILEMEMORY_H_

#include "memory/include/Memory.h"
#include "utils/include/Buffer.h"

#include <QFile>

class FileMemory : public Memory {
public:
	FileMemory();
	~FileMemory();
	bool open(const char *filename);
	void remove();
	void close();
	void setAddress(uint32_t address) override;
	uint32_t getAddress() override;
	void skip(uint32_t len) override;
	MemoryResult write(const void *pData, uint32_t len) override;
	MemoryResult read(void *pData, uint32_t size) override;
	uint32_t getMaxSize() const override;
	uint32_t getPageSize() const override;

protected:
	QFile file;
};

#endif
