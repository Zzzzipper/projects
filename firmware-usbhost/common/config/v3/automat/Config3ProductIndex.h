#ifndef COMMON_CONFIG_PRODUCTINDEX_H_
#define COMMON_CONFIG_PRODUCTINDEX_H_

#include "evadts/EvadtsProtocol.h"
#include "memory/include/Memory.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/List.h"

#pragma pack(push,1)
struct Config3ProductIndex {
	StrParam<EvadtsProductIdSize> selectId;
	uint16_t cashlessId;
};
#pragma pack(pop)

class Config3ProductIndexList {
public:
	static const uint16_t UndefinedIndex = 0xFFFF;

	MemoryResult init(Memory *memory);
	MemoryResult load(uint16_t productNum, Memory *memory);
	MemoryResult save(Memory *memory);
	void clear();
	bool add(const char *selectId, uint16_t cashlessId);
	uint16_t getIndex(const char *selectId);
	uint16_t getIndex(uint16_t cashlessId);
	uint16_t getSize() const { return list.getSize(); }
	Config3ProductIndex* get(uint16_t index);

private:
	List<Config3ProductIndex> list;
};

#endif
