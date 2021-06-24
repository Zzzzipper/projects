#ifndef COMMON_CONFIG_V1_FISCAL_CONVERTER_H_
#define COMMON_CONFIG_V1_FISCAL_CONVERTER_H_

#include "memory/include/Memory.h"
#include "Config1Fiscal.h"
#include "config/v2/fiscal/Config2Fiscal.h"

class ConfigFiscal1Converter {
public:
    ConfigFiscal1Converter();
	MemoryResult load(Memory *memory);
	MemoryResult save(Memory *memory);

private:
    Config1Fiscal fiscal1;
    Config2Fiscal fiscal2;
};

#endif
