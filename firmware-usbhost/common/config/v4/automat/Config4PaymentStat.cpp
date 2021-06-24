#include "Config4PaymentStat.h"

#include "memory/include/MemoryCrc.h"
#include "logger/include/Logger.h"

#include <memory.h>
#include <string.h>

MemoryResult Config4PaymentStat::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	memset(&data, 0, sizeof(data));
	return save();
}

MemoryResult Config4PaymentStat::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	MemoryCrc crc(memory);
	MemoryResult result = crc.readDataWithCrc(&data, sizeof(data));
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config4PaymentStat::save() {
	LOG_DEBUG(LOG_CFG, "saveData " << address);
	memory->setAddress(address);
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

MemoryResult Config4PaymentStat::reset() {
	LOG_DEBUG(LOG_CFG, "reset");
	data.cash_in_reset = 0;
	data.coin_in_cashbox_reset = 0;
	data.coin_in_tubes_reset = 0;
	data.bill_in_reset = 0;
	data.coin_dispense_reset = 0;
	data.coin_manually_dispense_reset = 0;
	data.cash_overpay_reset = 0;
	data.coin_filled_reset = 0;
	return save();
}

MemoryResult Config4PaymentStat::registerCoinIn(uint32_t value, uint8_t route) {
	if(route == MdbMasterCoinChanger::Route_Cashbox) {
		LOG_INFO(LOG_CFG, "coin in cashbox " << value);
		data.coin_in_cashbox_reset += value;
		data.coin_in_cashbox_total += value;
	} else if(route == MdbMasterCoinChanger::Route_Tube) {
		LOG_INFO(LOG_CFG, "coin in tube " << value);
		data.coin_in_tubes_reset += value;
		data.coin_in_tubes_total += value;
	} else {
		LOG_ERROR(LOG_CFG, "wrong route" << value);
		return MemoryResult_Ok;
	}
	data.cash_in_reset += value;
	data.cash_in_total += value;
	return save();
}

MemoryResult Config4PaymentStat::registerCoinDispense(uint32_t value) {
	LOG_INFO(LOG_CFG, "coin dispense " << value);
	data.coin_dispense_reset += value;
	data.coin_dispense_total += value;
	return save();
}

MemoryResult Config4PaymentStat::registerCoinDispenseManually(uint32_t value) {
	LOG_INFO(LOG_CFG, "coin dispense manually " << value);
	data.coin_dispense_reset += value;
	data.coin_dispense_total += value;
	data.coin_manually_dispense_reset += value;
	data.coin_manually_dispense_total += value;
	return save();
}

MemoryResult Config4PaymentStat::registerCoinFill(uint32_t value) {
	LOG_INFO(LOG_CFG, "coin fill " << value);
	data.coin_filled_reset += value;
	data.coin_filled_total += value;
	return save();
}

MemoryResult Config4PaymentStat::registerBillIn(uint32_t value) {
	LOG_INFO(LOG_CFG, "bill in " << value);
	data.bill_in_reset += value;
	data.bill_in_total += value;
	data.cash_in_reset += value;
	data.cash_in_total += value;
	return save();
}

MemoryResult Config4PaymentStat::registerCashOverpay(uint32_t value) {
	LOG_INFO(LOG_CFG, "overpay inc " << value);
	data.cash_overpay_reset += value;
	data.cash_overpay_total += value;
	return save();
}

uint32_t Config4PaymentStat::getDataSize() {
	return sizeof(Config4PaymentStatStruct);
}
