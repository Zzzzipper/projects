#include "RingList2.h"

#include "logger/include/Logger.h"

#include <string.h>

#define RINGLIST2_VERSION 3

#pragma pack(push,1)
struct RingList2Header {
	uint8_t  version;
	uint32_t dataSize;
	uint32_t size;
	uint8_t  crc[1];
};

struct RingList2Entry {
	uint32_t id;
	uint8_t  busy;
//	uint8_t  crc[1];
};
#pragma pack(pop)

RingList2::RingList2() :
	memory(NULL),
	address(0),
	dataSize(0),
	size(0),
	id(0),
	first(RINGLIST2_UNDEFINED),
	last(RINGLIST2_UNDEFINED)
{

}

MemoryResult RingList2::init(
	uint32_t dataSize,
	uint32_t size,
	Memory *memory
) {
	LOG_DEBUG(LOG_CFG, "init");
	MemoryResult result = initHeadData(dataSize, size, memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Init header failed");
		return result;
	}

	for(uint16_t i = 0; i < this->size; i++) {
		result = initEntryData(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Init entry failed");
			return result;
		}
	}

	return MemoryResult_Ok;
}

MemoryResult RingList2::insert(void *data, uint32_t dataLen) {
#if 0
	MemoryResult result = bind(getTail(), entry);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "bind failed" << result);
		return result;
	}
	entry->setId(id);
	entry->setBusy(1);
	result = entry->save();
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "save failed" << result);
		return result;
	}
	incrementPosition();
	return MemoryResult_Ok;
#else
	MemoryResult result = saveEntryData(data, dataLen);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "save failed" << result);
		return result;
	}
	incrementPosition();
	return MemoryResult_Ok;
#endif
}

MemoryResult RingList2::get(uint32_t index, void *buf, uint32_t bufSize) {
	if(memory == NULL) {
		LOG_ERROR(LOG_CFG, "Event list not inited");
		return MemoryResult_ProgError;
	}
	if(bufSize != dataSize) {
		LOG_ERROR(LOG_CFG, "Wrong entry size " << bufSize << "<>" << dataSize);
		return MemoryResult_ProgError;
	}
	if(index >= size) {
		LOG_ERROR(LOG_CFG, "Wrong index " << index << "," << size);
		return MemoryResult_OutOfIndex;
	}
	uint32_t entrySize = getEntrySize();
	uint32_t entryAddress = address + sizeof(RingList2Header) + index * entrySize;
	LOG_DEBUG(LOG_CFG, "get " << index << "," << entryAddress);
	memory->setAddress(entryAddress);
	return loadEntryData(buf, bufSize);
}

uint32_t RingList2::getSize() {
	return size;
}

uint32_t RingList2::getLen() {
	LOG_DEBUG(LOG_CFG, "getLen " << first << "," << last << "," << size);
	if(first == RINGLIST2_UNDEFINED) {
		return 0;
	}
	if(first <= last) {
		return (last - first + 1);
	} else {
		return (last + size - first + 1);
	}
}

uint32_t RingList2::getFirst() {
	return first;
}

uint32_t RingList2::getLast() {
	return last;
}

uint32_t RingList2::getTail() {
	uint32_t tail = last + 1;
	if(tail >= size) {
		tail = 0;
	}
	return tail;
}

uint32_t RingList2::getEntrySize() {
	return dataSize + sizeof(RingList2Entry) + MemoryCrc::getCrcSize();
}

MemoryResult RingList2::initHeadData(uint32_t dataSize, uint32_t size, Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	this->id = 1;
	this->dataSize = dataSize;
	this->size = size;
	this->first = RINGLIST2_UNDEFINED;
	this->last = RINGLIST2_UNDEFINED;
	return saveHeadData();
}

MemoryResult RingList2::saveHeadData() {
	LOG_DEBUG(LOG_CFG, "saveHeadData");
	memory->setAddress(address);
	RingList2Header header;
	header.version = RINGLIST2_VERSION;
	header.dataSize = dataSize;
	header.size = size;
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&header, sizeof(header));
}

MemoryResult RingList2::initEntryData(Memory *memory) {
	uint8_t data[dataSize];
	memset(data, 0, dataSize);
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.write(&data, dataSize);
	if(result != MemoryResult_Ok) {
		return result;
	}
	RingList2Entry entry;
	entry.id = RINGLIST2_UNDEFINED;
	entry.busy = 0;
	result = crc.write(&entry, sizeof(entry));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return crc.writeCrc();
}

MemoryResult RingList2::saveEntryData(void *data, uint32_t dataLen) {
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.write(data, dataLen);
	if(result != MemoryResult_Ok) {
		return result;
	}
	RingList2Entry entry;
	entry.id = id;
	entry.busy = 1;
	result = crc.write(&entry, sizeof(entry));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return crc.writeCrc();
}

MemoryResult RingList2::loadEntryData(void *buf, uint32_t bufSize) {
	MemoryCrc crc(memory);
	crc.startCrc();
	MemoryResult result = crc.read(buf, bufSize);
	if(result != MemoryResult_Ok) {
		return result;
	}
	RingList2Entry entry;
	result = crc.read(&entry, sizeof(entry));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return crc.readCrc();
}

void RingList2::incrementPosition() {
	if(last == RINGLIST2_UNDEFINED) {
		last = 0;
	} else {
		last++;
		if(last >= size) {
			last = 0;
		}
	}

	if(first == RINGLIST2_UNDEFINED) {
		first = 0;
	} else {
		if(last == first) {
			first++;
			if(first >= size) {
				first = 0;
			}
		}
	}

	LOG_DEBUG(LOG_CFG, "incrementPosition " << first << "," << last);
}

/*
MemoryResult RingList2::bind(uint32_t index, RingListEntry *entry) {
#if 0
	if(memory == NULL) {
		LOG_ERROR(LOG_CFG, "Event list not inited");
		return MemoryResult_ProgError;
	}
	if(index >= size) {
		LOG_ERROR(LOG_CFG, "Wrong index " << index << "," << size);
		return MemoryResult_OutOfIndex;
	}
	uint32_t entrySize = entry->getDataSize();
	uint32_t entryAddress = address + sizeof(RingListHeader) + index * entrySize;
	LOG_DEBUG(LOG_CFG, "bind " << index << "," << entryAddress);
	memory->setAddress(entryAddress);
	entry->bind(memory);
	return MemoryResult_Ok;
#else
	if(memory == NULL) {
		LOG_ERROR(LOG_CFG, "Event list not inited");
		return MemoryResult_ProgError;
	}
	if(index >= size) {
		LOG_ERROR(LOG_CFG, "Wrong index " << index << "," << size);
		return MemoryResult_OutOfIndex;
	}
	uint32_t entrySize = entry->getDataSize();
	uint32_t entryAddress = address + sizeof(RingListHeader) + index * entrySize;
	LOG_DEBUG(LOG_CFG, "bind " << index << "," << entryAddress);
	memory->setAddress(entryAddress);
	entry->bind(memory);
	return MemoryResult_Ok;
#endif
}
*/
