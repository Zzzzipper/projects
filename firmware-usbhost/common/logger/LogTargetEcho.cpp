#include "include/LogTargetEcho.h"
#include "uart/include/interface.h"

LogTargetEcho::LogTargetEcho(uint8_t *buf, uint32_t bufSize) :
	buf(buf),
	bufSize(bufSize),
	index(0)
{
}

void LogTargetEcho::send(const uint8_t *data, const uint16_t len) {
	for(uint16_t i = 0; i < len; i++) {
		buf[index] = data[i];
		index++;
		if(index >= bufSize) { index = 0; }
	}
}
