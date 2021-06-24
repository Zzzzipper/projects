#include "Config1FiscalConverter.h"
#include "mdb/MdbProtocol.h"
#include "logger/include/Logger.h"

ConfigFiscal1Converter::ConfigFiscal1Converter() {

}

MemoryResult ConfigFiscal1Converter::load(Memory *memory) {
	MemoryResult result = fiscal1.load(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult ConfigFiscal1Converter::save(Memory *memory) {
	MemoryResult result = fiscal2.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}

	uint32_t address = memory->getAddress();
	fiscal2.setKkt(fiscal1.getKkt());
	fiscal2.setKktInterface(fiscal1.getKktInterface());
	fiscal2.setKktAddr(fiscal1.getKktAddr());
	fiscal2.setKktPort(fiscal1.getKktPort());
	fiscal2.setOfdAddr(fiscal1.getOfdAddr());
	fiscal2.setOfdPort(fiscal1.getOfdPort());
	result = fiscal2.save();
	if(result != MemoryResult_Ok) {
		return result;
	}

	memory->setAddress(address);
	return MemoryResult_Ok;
}
