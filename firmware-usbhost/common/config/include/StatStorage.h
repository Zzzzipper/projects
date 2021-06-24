#ifndef COMMON_CONFIG_STATSTORAGE_H_
#define COMMON_CONFIG_STATSTORAGE_H_

#include "utils/include/List.h"

class StatNode {
public:
	StatNode(uint16_t id, uint32_t startValue);
	virtual ~StatNode();
	uint16_t getId();
	virtual void reset();
	virtual void set(uint32_t value);
	virtual void max(uint32_t max);
	virtual void inc();
	uint32_t get();

private:
	uint16_t id;
	uint32_t value;
};

class StatStorage {
public:
	StatNode *add(uint16_t id, uint32_t startValue);
	StatNode *get(uint16_t id);
	StatNode *getByIndex(uint16_t index);

private:
	List<StatNode> nodes;

};

#endif
