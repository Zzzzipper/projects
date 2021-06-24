#include "include/FlashMemory.h"

#include "common/logger/include/Logger.h"

#include "../defines.h"

#define STM32F04_SECTOR_NOT_FOUND	0xFFFF
#define STM32F04_SECTOR_MIN_SIZE	0x4000 // 16 килобайт

FlashMemory::FlashMemory() : m_lock(true) {

}

FlashMemory::~FlashMemory() {
	lock();
}

/*
 * Прежде чем записать хотя бы один байт во флеш память её надо стереть.
 * Обратите внимание что стирается память секторами!
 * То есть если вам нужно поправить пару байт, то нужно зачитать
 * содержимое сектора в ОЗУ, исправить данные и сохранить сектор целиком
 * обратно.
 */
bool FlashMemory::start(uint32_t startAddress, uint32_t dataSize, bool checkWrited) {
	unlock();
	if(erase(startAddress, dataSize) == false) {
		LOG_ERROR(LOG_MODEM, "eraseData failed");
		lock();
		return false;
	}

	m_checkWrited = checkWrited;
	startAddr = (uint8_t*)startAddress;
	curAddr = (uint8_t*)startAddress;
	endAddr = (uint8_t*)(startAddress + dataSize - 1);
	return true;
}

/*
 * Функция пишет данные друг за другом, начиная со startAddress.
 * То есть после каждой записи адрес назначения смещается на длину записанных данных.
 * Параметры:
 *   data - указатель на данные
 *   dataLen - длина записываемых данных
 * Результат:
 *   true - данные успешно записаны
 *   false - ошибка записи
 */
bool FlashMemory::write(const uint8_t *data, uint32_t dataLen) {
	LOG_INFO(LOG_MODEM, "writeData addr=" << (uint32_t)curAddr << ", dataLen=" << dataLen);
	if(m_lock == true) {
		LOG_ERROR(LOG_MODEM, "Write before start");
		return false;
	}
	if(endAddr < (curAddr + dataLen - 1)) {
		LOG_ERROR(LOG_MODEM, "Addresses out of erased memory");
		return false;
	}
	uint32_t dogReset = 1024;
	uint8_t *src = (uint8_t*)data;
	uint8_t *dst = (uint8_t*)curAddr;
	for(uint32_t i = 0; i < dataLen; i++) {
		FLASH_Status status = FLASH_ProgramByte((uint32_t)dst, *src);
		if(status != FLASH_COMPLETE) {
			LOG_ERROR(LOG_MODEM, "Memory write error " << status);
			return false;
		}
		src++;
		dst++;
		dogReset--;
		if(dogReset == 0) {
			IWDG_ReloadCounter();
			dogReset = 1024;
		}
	}

	if(m_checkWrited == true) {
		uint32_t errors = check((uint32_t)curAddr, data, dataLen);
		if(errors == 0) {
			LOG_ERROR(LOG_MODEM, "Error " << (uint32_t)curAddr << " errors=" << errors);
			return false;
		}
	}

	curAddr = curAddr + dataLen;
	return true;
}

void FlashMemory::stop() {
	lock();
}

uint32_t FlashMemory::check(uint32_t startAddress, const uint8_t *data, uint32_t dataLen) {
	uint8_t *src = (uint8_t*)data;
	uint8_t *dst = (uint8_t*)startAddress;
	uint32_t errors = 0;
	for(uint32_t i = 0; i < dataLen; i++) {
		if(*src != *dst) {
			errors++;
		}
		src++;
		dst++;
	}
	return errors;
}

void FlashMemory::lock() {
	if(m_lock == false) {
		FLASH_Lock();
		m_lock = true;
	}
}

void FlashMemory::unlock() {
	if(m_lock == true) {
		FLASH_Unlock();
		m_lock = false;
	}
}

/*
 * Возвращает идентификатор сектора флеш памяти по адресу.
 * Результат:
 *   идентификатор сектор - сектор найден;
 *   STM32F04_SECTOR_NOT_FOUND - адрес выходит за границиы секторов флеш памяти.
 */
uint16_t FlashMemory::getSectorIdByAddress(uint32_t startAddress) {
	struct {
		uint32_t from;
		uint32_t to;
		uint16_t id;
	} sectors[] = {
		{ 0x08000000, 0x08003FFF, FLASH_Sector_0 },
		{ 0x08004000, 0x08007FFF, FLASH_Sector_1 },
		{ 0x08008000, 0x0800BFFF, FLASH_Sector_2 },
		{ 0x0800C000, 0x0800FFFF, FLASH_Sector_3 },
		{ 0x08010000, 0x0801FFFF, FLASH_Sector_4 },
		{ 0x08020000, 0x0803FFFF, FLASH_Sector_5 },
		{ 0x08040000, 0x0805FFFF, FLASH_Sector_6 },
		{ 0x08060000, 0x0807FFFF, FLASH_Sector_7 },
		{ 0x08080000, 0x0809FFFF, FLASH_Sector_8 },
		{ 0x080A0000, 0x080BFFFF, FLASH_Sector_9 },
		{ 0x080C0000, 0x080DFFFF, FLASH_Sector_10 },
		{ 0x080E0000, 0x080FFFFF, FLASH_Sector_11 },
		{ 0, 0, STM32F04_SECTOR_NOT_FOUND },
	};
	for(int i = 0; sectors[i].id != STM32F04_SECTOR_NOT_FOUND; i++) {
		if(sectors[i].from <= startAddress && startAddress <= sectors[i].to) {
			LOG_INFO(LOG_MODEM, "Found sector " << sectors[i].id << " " << sectors[i].from << "-" << sectors[i].to);
			return sectors[i].id;
		}
	}
	return STM32F04_SECTOR_NOT_FOUND;
}

/*
 * Очищает все сектора, в которые планируется писать данные.
 * На STM32F04 прежде чем писать во флэшпамять необходимо их стереть.
 */
bool FlashMemory::erase(uint32_t startAddress, uint32_t dataSize) {
	uint32_t endAddress = startAddress + dataSize - 1;
	if(getSectorIdByAddress(startAddress) == STM32F04_SECTOR_NOT_FOUND) {
		LOG_ERROR(LOG_MODEM, "Addresses out of flash memory");
		return false;
	}
	if(getSectorIdByAddress(endAddress) == STM32F04_SECTOR_NOT_FOUND) {
		LOG_ERROR(LOG_MODEM, "Addresses out of flash memory");
		return false;
	}

	// чистим все сектора двигаясь по адресному пространству с шагом равным минимальному размеру сектора
	uint16_t curSectorId = STM32F04_SECTOR_NOT_FOUND;
	uint16_t newSectorId = STM32F04_SECTOR_NOT_FOUND;
	for(uint32_t addr = startAddress; addr < endAddress;) {
		newSectorId = getSectorIdByAddress(addr);
		if(newSectorId != curSectorId) {
			LOG_INFO(LOG_MODEM, "Erase sector " << newSectorId);
			FLASH_Status status = FLASH_EraseSector(newSectorId, VoltageRange_3);
			if(status != FLASH_COMPLETE) {
				LOG_ERROR(LOG_MODEM, "Memory erase failed " << status);
				return false;
			}
			curSectorId = newSectorId;
			IWDG_ReloadCounter();
		}
		addr += STM32F04_SECTOR_MIN_SIZE;
	}

	// обязательно проверяем сектор последнего байта диапазона
	newSectorId = getSectorIdByAddress(endAddress);
	if(newSectorId != curSectorId) {
		FLASH_EraseSector(newSectorId, VoltageRange_3);
		IWDG_ReloadCounter();
	}

	return true;
}
