#ifndef COMMON_CONFIG_PRODUCTINDEX1_H_
#define COMMON_CONFIG_PRODUCTINDEX1_H_

#include "Evadts1Protocol.h"
#include "memory/include/Memory.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/List.h"

#pragma pack(push,1)
struct Config1ProductIndex {
	StrParam<Evadts1ProductIdSize> selectId;
	uint16_t cashlessId;

	static bool selectId2cashlessId(const char *selectId, uint16_t *cashlessId);
};
#pragma pack(pop)

class Config1ProductIndexList {
public:
	static const uint16_t UndefinedIndex = 0xFFFF;

	void init(Memory *memory);
	MemoryResult save(Memory *memory);
	MemoryResult load(uint16_t productNum, Memory *memory);
	void clear();
	bool add(const char *selectId);
	bool add(const char *selectId, uint16_t cashlessId);
	uint16_t getIndex(const char *selectId);
	uint16_t getIndex(uint16_t cashlessId);
	uint16_t getSize() const { return list.getSize(); }
	Config1ProductIndex* get(uint16_t index);

private:
	List<Config1ProductIndex> list;
};

#endif
