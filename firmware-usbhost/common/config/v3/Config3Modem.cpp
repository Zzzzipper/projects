#include "Config3Modem.h"
#include "logger/include/Logger.h"

#define EVENT_LIST_SIZE 100

Config3Modem::Config3Modem(Memory *memory, RealTimeInterface *realtime, StatStorage *stat) :
	memory(memory),
	realtime(realtime),
	stat(stat),
	address(0),
	events(realtime),
	automat(realtime)
{
	product = automat.createIterator();
}

Config3Modem::~Config3Modem() {
	delete product;
}

MemoryResult Config3Modem::init() {
	LOG_DEBUG(LOG_CFG, "init");
	memory->setAddress(address);
	MemoryResult result = boot.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}

	result = fiscal.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}

	result = events.init(EVENT_LIST_SIZE, memory);
	if(result != MemoryResult_Ok) {
		return result;
	}

	result = automat.init(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}

	return MemoryResult_Ok;
}

MemoryResult Config3Modem::load() {
	LOG_DEBUG(LOG_CFG, "load");
	memory->setAddress(address);
	MemoryResult result = boot.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Boot config load failed");
		return result;
	}

	result = fiscal.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Fiscal config load failed");
		return result;
	}

	result = events.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Events config load failed");
		return result;
	}

	result = automat.load(memory);
	if(result != MemoryResult_Ok) {
		LOG_ERROR(LOG_CFG, "Automat config load failed");
		return result;
	}

	LOG_INFO(LOG_CFG, "Config load OK");
	return MemoryResult_Ok;
}

MemoryResult Config3Modem::save() {
	LOG_DEBUG(LOG_CFG, "save");
	MemoryResult result = boot.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = fiscal.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = events.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	result = automat.save();
	if(result != MemoryResult_Ok) {
		return result;
	}
	return MemoryResult_Ok;
}

MemoryResult Config3Modem::reinit() {
	LOG_DEBUG(LOG_CFG, "reinit");
	memory->setAddress(address);
	uint32_t start = memory->getAddress();
	MemoryResult result = boot.load(memory);
	if(result != MemoryResult_Ok) {
		memory->setAddress(start);
		result = boot.init(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
		LOG_INFO(LOG_CFG, "Boot config inited");
	}

	start = memory->getAddress();
	result = fiscal.load(memory);
	if(result != MemoryResult_Ok) {
		memory->setAddress(start);
		result = fiscal.init(memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
		LOG_INFO(LOG_CFG, "Fiscal config inited");
	}

	start = memory->getAddress();
	result = events.load(memory);
	if(result != MemoryResult_Ok) {
		memory->setAddress(start);
		result = events.init(EVENT_LIST_SIZE, memory);
		if(result != MemoryResult_Ok) {
			return result;
		}
		LOG_INFO(LOG_CFG, "Events config inited");
	}

	result = automat.reinit(memory);
	if(result != MemoryResult_Ok) {
		return result;
	}

	LOG_INFO(LOG_CFG, "Config reinited");
	return MemoryResult_Ok;
}

Evadts::Result Config3Modem::comparePlanogram(Config3PriceIndexList *prices, Config3ProductIndexList *products) {
	if(asyncComparePlanogramStart(prices, products) == false) {
		return asyncComparePlanogramResult();
	}
	while(asyncComparePlanogramProc() == true) {}
	return asyncComparePlanogramResult();
}

bool Config3Modem::asyncComparePlanogramStart(Config3PriceIndexList *prices, Config3ProductIndexList *products) {
	this->products = products;

	if(automat.getPriceListNum() != prices->getSize()) {
		LOG_ERROR(LOG_CFG, "Price list num not equal " << automat.getPriceListNum() << "<>" << prices->getSize());
		this->num = products->getSize();
		this->result2 = Evadts::Result_PriceListNumberNotEqual;
		return false;
	}
	if(automat.getProductNum() != products->getSize()) {
		LOG_ERROR(LOG_CFG, "Products num not equal " << automat.getProductNum() << "<>" << products->getSize());
		this->num = products->getSize();
		this->result2 = Evadts::Result_ProductNumberNotEqual;
		return false;
	}

	if(product->first() == false) {
		LOG_ERROR(LOG_CFG, "Product list is empty");
		this->num = products->getSize();
		this->result2 = Evadts::Result_OK;
		return false;
	}

	for(uint16_t i = 0; i < prices->getSize(); i++) {
		Config3PriceIndex *index = prices->get(i);
		if(product->getPrice(index->device.get(), index->number) == NULL) {
			LOG_ERROR(LOG_CFG, "Price list not found " << index->device.get() << index->number);
			this->num = products->getSize();
			this->result2 = Evadts::Result_PriceListNotFound;
			return false;
		}
	}

	this->num = 0;
	this->result2 = Evadts::Result_Busy;
	return true;
}

bool Config3Modem::asyncComparePlanogramProc() {
	if(num >= products->getSize()) {
		return false;
	}

	Config3ProductIndex *index = products->get(num);
	if(product->findBySelectId(index->selectId.get()) == false) {
		LOG_ERROR(LOG_CFG, "Select id not found " << index->selectId.get());
		num = products->getSize();
		result2 = Evadts::Result_ProductNotFound;
		return false;
	}

	if(index->cashlessId != Config3ProductIndexList::UndefinedIndex) {
		if(product->getCashlessId() != index->cashlessId) {
			LOG_ERROR(LOG_CFG, "Cashless id not equal " << index->cashlessId);
			num = products->getSize();
			result2 = Evadts::Result_CashlessIdNotEqual;
			return false;
		}
	}

	num++;
	if(num >= products->getSize()) {
		result2 = Evadts::Result_OK;
		return false;
	}

	return true;
}

Evadts::Result Config3Modem::asyncComparePlanogramResult() {
	return result2;
}

bool Config3Modem::isResizeable(Config3PriceIndexList *prices, Config3ProductIndexList *products) {
	(void)prices;
	for(uint16_t i = 0; i < products->getSize(); i++) {
		Config3ProductIndex *index = products->get(i);
		if(index->cashlessId == Config3ProductIndexList::UndefinedIndex) {
			return false;
		}
	}
	return true;
}

Evadts::Result Config3Modem::resizePlanogram(Config3PriceIndexList *prices, Config3ProductIndexList *products) {
	if(asyncResizePlanogramStart(prices, products) == false) {
		return asyncResizePlanogramResult();
	}
	while(asyncResizePlanogramProc() == true) {}
	return asyncResizePlanogramResult();
}

bool Config3Modem::asyncResizePlanogramStart(Config3PriceIndexList *prices, Config3ProductIndexList *products) {
	LOG_INFO(LOG_CFG, "asyncResizePlanogramStart " << address);
	memory->setAddress(address);
	uint32_t start = memory->getAddress();
	MemoryResult result = boot.load(memory);
	if(result != MemoryResult_Ok) {
		memory->setAddress(start);
		result = boot.init(memory);
		if(result != MemoryResult_Ok) {
			result2 = Evadts::Result_RomWriteError;
			return false;
		}
		LOG_INFO(LOG_CFG, "Boot config inited");
	}

	start = memory->getAddress();
	result = fiscal.load(memory);
	if(result != MemoryResult_Ok) {
		memory->setAddress(start);
		result = fiscal.init(memory);
		if(result != MemoryResult_Ok) {
			result2 = Evadts::Result_RomWriteError;
			return false;
		}
		LOG_INFO(LOG_CFG, "Fiscal config inited");
	}

	start = memory->getAddress();
	result = events.load(memory);
	if(result != MemoryResult_Ok) {
		memory->setAddress(start);
		result = events.init(EVENT_LIST_SIZE, memory);
		if(result != MemoryResult_Ok) {
			result2 = Evadts::Result_RomWriteError;
			return false;
		}
		LOG_INFO(LOG_CFG, "Events config inited");
	}

	automat.shutdown();
	for(uint16_t i = 0; i < prices->getSize(); i++) {
		Config3PriceIndex *index = prices->get(i);
		automat.addPriceList(index->device.get(), index->number, (Config3PriceIndexType)index->type);
	}
	for(uint16_t i = 0; i < products->getSize(); i++) {
		Config3ProductIndex *index = products->get(i);
		automat.addProduct(index->selectId.get(), index->cashlessId);
	}
	automat.asyncInitStart(memory);
	result2 = Evadts::Result_Busy;
	return true;
}

bool Config3Modem::asyncResizePlanogramProc() {
	LOG_DEBUG(LOG_CFG, "asyncResizePlanogramProc");
	if(automat.asyncInitProc() == true) {
		return true;
	}

	LOG_INFO(LOG_CFG, "Automat config inited");
	result2 = Evadts::Result_OK;
	return false;
}

Evadts::Result Config3Modem::asyncResizePlanogramResult() {
	return result2;
}
