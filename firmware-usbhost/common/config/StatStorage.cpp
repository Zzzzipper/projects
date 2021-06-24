#include "config/include/StatStorage.h"
#include "logger/include/Logger.h"

StatNode::StatNode(uint16_t id, uint32_t startValue) : id(id), value(startValue) {

}

StatNode::~StatNode() {

}

uint16_t StatNode::getId() {
	return id;
}

void StatNode::reset() {
	value = 0;
}

void StatNode::set(uint32_t value) {
	this->value = value;
}

void StatNode::max(uint32_t max) {
	if(value < max) { value = max; }
}

void StatNode::inc() {
	value++;
}

uint32_t StatNode::get() {
	return value;
}

StatNode *StatStorage::add(uint16_t id, uint32_t startValue) {
	StatNode *node = get(id);
	if(node != NULL) {
		return node;
	}
	node = new StatNode(id, startValue);
	nodes.add(node);
	return node;
}

StatNode *StatStorage::get(uint16_t id) {
	for(uint16_t i = 0; i < nodes.getSize(); i++) {
		StatNode *node = nodes.get(i);
		if(node->getId() == id) {
			return node;
		}
	}
	return NULL;
}

StatNode *StatStorage::getByIndex(uint16_t index) {
	return nodes.get(index);
}
