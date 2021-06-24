#ifndef COMMON_CONFIG_EVENT_RINGLIST_H_
#define COMMON_CONFIG_EVENT_RINGLIST_H_

#include "memory/include/Memory.h"
#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#define RINGLIST_UNDEFINED 0xFFFFFFFF
#define RINGLIST_VERSION 3

#pragma pack(push,1)
struct RingListHeader {
	uint8_t version;
	uint32_t size;
	uint8_t  crc[1];
};
#pragma pack(pop)

template <class RingListEntry>
class RingList {
public:
	RingList() {}
	MemoryResult init(uint32_t size, Memory *memory) {
		LOG_DEBUG(LOG_CFG, "init");
		MemoryResult result = initHeadData(size, memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Init header failed");
			return result;
		}

		RingListEntry entry;
		entry.setId(0);
		entry.setBusy(0);
		for(uint16_t i = 0; i < this->size; i++) {
			result = entry.init(memory);
			if(result != MemoryResult_Ok) {
				LOG_ERROR(LOG_CFG, "Init entry failed");
				return result;
			}
		}
		return MemoryResult_Ok;
	}

	MemoryResult loadAndCheck(Memory *memory) {
		LOG_DEBUG(LOG_CFG, "loadAndCheck");
		MemoryResult result = loadHeadData(memory);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Load header failed");
			return result;
		}

		id = 0;
		last = 0;
		RingListEntry entry;
		for(uint32_t i = 0; i < size; i++) {
			result = entry.load(memory);
			if(result != MemoryResult_Ok) {
				LOG_ERROR(LOG_CFG, "Load entry failed " << i);
				continue;
			}
			if(entry.getId() > id) {
				id = entry.getId();
				last = i;
			}
		}

		if(id <= 0) {
			first = RINGLIST_UNDEFINED;
			last = RINGLIST_UNDEFINED;
			return MemoryResult_Ok;
		}

		first = RINGLIST_UNDEFINED;
		for(uint32_t i = last;;) {
			result = get(i, &entry);
			if(result != MemoryResult_Ok) {
				LOG_ERROR(LOG_CFG, "Load entry failed " << i);
				return result;
			}
			if(entry.getBusy() <= 0) {
				break;
			}
			first = i;
			if(i == 0) {
				i = size - 1;
			} else {
				i--;
			}
			if(i == last) {
				break;
			}
		}
		return MemoryResult_Ok;
	}

	MemoryResult save() {
		return MemoryResult_Ok;
	}

	MemoryResult clear() {
		return MemoryResult_Ok;
	}

	uint32_t getSize() { return size; }
	uint32_t getLen() {
		LOG_DEBUG(LOG_CFG, "getLen " << first << "," << last << "," << size);
		if(first == RINGLIST_UNDEFINED) {
			return 0;
		}
		if(first <= last) {
			return (last - first + 1);
		} else {
			return (last + size - first + 1);
		}
	}
	uint32_t getFirst() { return first; }
	uint32_t getLast() { return last; }
	uint32_t getTail() {
		uint32_t tail = last + 1;
		if(tail >= size) {
			tail = 0;
		}
		return tail;
	}

	MemoryResult insert(RingListEntry *entry) {
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
	}

	MemoryResult get(uint32_t index, RingListEntry *entry) {
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
		LOG_DEBUG(LOG_CFG, "get " << index << "," << entryAddress);
		memory->setAddress(entryAddress);
		return entry->load(memory);
	}

	MemoryResult remove(uint32_t index) {
		RingListEntry entry;
		MemoryResult result = get(index, &entry);
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Load entry failed " << index);
			return result;
		}
		entry.setBusy(0);
		result = entry.save();
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "save failed" << result);
			return result;
		}
		if(last == index) {
			first = RINGLIST_UNDEFINED;
		} else {
			first = index + 1;
			if(first >= size) {
				first = 0;
			}
		}
		return MemoryResult_Ok;
	}

/*
	MemoryResult load(Memory *memory);
	MemoryResult loadAndCheck(Memory *memory);
	MemoryResult save();
	MemoryResult clear();
	MemoryResult repair(int16_t size, Memory *memory);

	bool getFromFirst(uint16_t index, ConfigEvent *event);
*/

private:
	Memory *memory;
	uint32_t address;
	uint32_t size;
	uint32_t id;
	uint32_t first;
	uint32_t last;

	MemoryResult initHeadData(uint32_t size, Memory *memory) {
		this->memory = memory;
		this->address = memory->getAddress();
		this->id = 1;
		this->size = size;
		this->first = RINGLIST_UNDEFINED;
		this->last = RINGLIST_UNDEFINED;
		return saveHeadData();
	}

	MemoryResult loadHeadData(Memory *memory) {
		this->memory = memory;
		this->address = memory->getAddress();
		MemoryCrc crc(memory);
		RingListHeader header;
		MemoryResult result = crc.readDataWithCrc(&header, sizeof(header));
		if(result != MemoryResult_Ok) {
			LOG_ERROR(LOG_CFG, "Wrong entry list CRC");
			return result;
		}
		if(header.version != RINGLIST_VERSION) {
			LOG_ERROR(LOG_CFG, "Wrong entry list version " << header.version);
			return MemoryResult_WrongVersion;
		}
		this->size = header.size;
		return MemoryResult_Ok;
	}

	MemoryResult saveHeadData() {
		LOG_DEBUG(LOG_CFG, "saveHeadData");
		memory->setAddress(address);
		RingListHeader header;
		header.version = RINGLIST_VERSION;
		header.size = size;
		MemoryCrc crc(memory);
		return crc.writeDataWithCrc(&header, sizeof(header));
	}

	MemoryResult bind(uint32_t index, RingListEntry *entry) {
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
	}

	void incrementPosition() {
		if(last == RINGLIST_UNDEFINED) {
			last = 0;
		} else {
			last++;
			if(last >= size) {
				last = 0;
			}
		}

		if(first == RINGLIST_UNDEFINED) {
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
	static MemoryResult loadData(Memory *memory, ConfigEventListData *data);*/
};

#endif
