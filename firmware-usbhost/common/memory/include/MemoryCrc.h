/*
 * Crc.h
 *
 * Created: 22.05.2014 16:06:39
 *  Author: Vladimir
 */

#ifndef CRC_H_
#define CRC_H_

#include "memory/include/Memory.h"

class MemoryCrc {
public:
	MemoryCrc(Memory *memory);
	void startCrc();
	MemoryResult write(const void *data, uint16_t len);
	MemoryResult writeCrc();
	MemoryResult read(void *buf, uint16_t size);
	MemoryResult readCrc();
	MemoryResult writeDataWithCrc(const void *block, uint16_t blockSize);
	MemoryResult readDataWithCrc(void *block, uint16_t blockSize);
	static uint32_t getCrcSize();

private:
	Memory *memory;
	uint8_t crc;

	void calcCrc(const uint8_t *data, uint16_t len);
};

#endif /* CRC_H_ */
