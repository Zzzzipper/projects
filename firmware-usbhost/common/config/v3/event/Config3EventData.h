#ifndef COMMON_CONFIG_V3_EVENTDATA_H_
#define COMMON_CONFIG_V3_EVENTDATA_H_

#include "utils/include/NetworkProtocol.h"
#include "timer/include/DateTime.h"
#include "evadts/EvadtsProtocol.h"

#define CONFIG_EVENTLIST_VERSION 2

#pragma pack(push,1)
struct Config3EventListData {
	uint8_t version;
	uint16_t size;
	uint16_t first;
	uint16_t last;
	uint16_t sync;
	uint8_t  crc[1];

	void set(Config3EventListData *data) {
		this->version = data->version;
		this->size = data->size;
		this->first = data->first;
		this->last = data->last;
		this->sync = data->sync;
	}
};

struct Config3EventData {
	uint32_t number;
	StrParam<50> string;
};

struct Config3EventSale {
	StrParam<EvadtsProductIdSize> selectId;
	uint32_t wareId;
	StrParam<ConfigProductNameSize> name;
	StrParam<EvadtsPaymentDeviceSize> device;
	uint8_t  priceList;
	uint32_t price;
	uint8_t  taxSystem;
	uint8_t	 taxRate;
	uint32_t taxValue;
	uint64_t fiscalRegister; // ФР (номер фискального регистратора)
	uint64_t fiscalStorage;  // ФН (номер фискального накопителя)
	uint32_t fiscalDocument; // ФД (номер фискального документа)
	uint32_t fiscalSign;     // ФП или ФПД (фискальный признак документа)

	void set(Config3EventSale *entry);
};

struct Config3EventStruct {
	DateTime date;
	uint16_t code;
	union {
		Config3EventData data;
		Config3EventSale sale;
	};
	uint8_t  crc[1];

	Config3EventStruct() {}
};
#pragma pack(pop)

#endif
