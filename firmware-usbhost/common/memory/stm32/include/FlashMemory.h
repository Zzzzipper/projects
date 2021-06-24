#ifndef COMMON_MEMORY_STM32_FLASHMEMORY_H
#define COMMON_MEMORY_STM32_FLASHMEMORY_H

#include <stdint.h>

/*
 * Перезаписывает флэш-память.
 * Более одного экземпляра лучше не создавать.
 * Если запустить две параллельные записи в память, то результат не гарантируется.
 */
class FlashMemory {
public:
	FlashMemory();
	~FlashMemory();
	bool start(uint32_t startAddress, uint32_t dataSize, bool checkWrited = false);
	bool write(const uint8_t *data, uint32_t dataLen);
	void stop();
	uint32_t check(uint32_t startAddress, const uint8_t *data, uint32_t dataLen);

private:
	bool m_lock;
	bool m_checkWrited;
	uint8_t *startAddr;
	uint8_t *curAddr;
	uint8_t *endAddr;

	void lock();
	void unlock();
	uint16_t getSectorIdByAddress(uint32_t startAddress);
	bool erase(uint32_t startAddress, uint32_t dataSize);
};

#endif
