#include "Config2EventList.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"
#include "platform/include/platform.h"

#include <string.h>
#include <strings.h>

Config2Event::Config2Event() {

}

MemoryResult Config2Event::load(Memory *memory) {
	MemoryCrc crc(memory);
	return crc.readDataWithCrc(&data, sizeof(data));
}

MemoryResult Config2Event::save(Memory *memory) {
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}
