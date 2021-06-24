#ifndef COMMON_CONFIG_V4_EVENTDATA_H_
#define COMMON_CONFIG_V4_EVENTDATA_H_

#include "utils/include/NetworkProtocol.h"
#include "timer/include/DateTime.h"
#include "evadts/EvadtsProtocol.h"

#define CONFIG_EVENTLIST_VERSION 2

#pragma pack(push,1)
struct Config4EventListData {
	uint8_t version;
	uint16_t size;
	uint16_t first;
	uint16_t last;
	uint16_t sync;
	uint8_t  crc[1];

	void set(Config4EventListData *data) {
		this->version = data->version;
		this->size = data->size;
		this->first = data->first;
		this->last = data->last;
		this->sync = data->sync;
	}
};

struct Config4EventData {
	StrParam<50> string;
};

struct Config4EventSale {
	StrParam<EvadtsProductIdSize> selectId;
	uint32_t wareId;
	StrParam<ConfigProductNameSize> name;
	StrParam<EvadtsPaymentDeviceSize> device;
	uint8_t  priceList;
	uint32_t price;
	uint8_t  taxSystem;
	uint8_t	 taxRate;
	uint32_t taxValue;
	uint8_t  loyalityType;
	BinParam<uint8_t, LOYALITY_CODE_SIZE> loyalityCode;
	DateTime fiscalDatetime;
	uint64_t fiscalRegister; // ФР (номер фискального регистратора)
	uint64_t fiscalStorage;  // ФН (номер фискального накопителя)
	uint32_t fiscalDocument; // ФД (номер фискального документа)
	uint32_t fiscalSign;     // ФП или ФПД (фискальный признак документа)

	void set(Config4EventSale *entry);
};

struct Config4EventStruct {
	uint32_t id;
	DateTime date;
	uint16_t code;
	union {
		Config4EventData data;
		Config4EventSale sale;
	};
	uint8_t busy;
	uint8_t crc[1];

	Config4EventStruct() {}
};
#pragma pack(pop)

#endif
