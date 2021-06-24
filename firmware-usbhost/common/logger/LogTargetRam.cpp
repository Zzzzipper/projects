#include "include/LogTargetRam.h"

LogTargetRam::LogTargetRam(uint16_t size) : buf(size) {}

void LogTargetRam::send(const uint8_t *data, const uint16_t len) {
	buf.add(data, len);
}

void LogTargetRam::clear() {
	buf.clear();
}

uint8_t *LogTargetRam::getData() {
	return buf.getData();
}

uint16_t LogTargetRam::getLen() {
	return buf.getLen();
}
