#include <string.h>
#include <strings.h>

#include "Config4EventList.h"
#include "memory/include/MemoryCrc.h"
#include "utils/include/StringParser.h"
#include "logger/include/Logger.h"
#include "platform/include/platform.h"


void Config4EventSale::set(Config4EventSale *data) {
	selectId.set(data->selectId.get());
	wareId = data->wareId;
	name.set(data->name.get());
	device.set(data->device.get());
	priceList = data->priceList;
	price = data->price;
	taxSystem = data->taxSystem;
	taxRate = data->taxRate;
	taxValue = data->taxValue;
	loyalityType = data->loyalityType;
	loyalityCode.set(data->loyalityCode.getData(), data->loyalityCode.getLen());
	fiscalRegister = data->fiscalRegister;
	fiscalStorage = data->fiscalStorage;
	fiscalDocument = data->fiscalDocument;
	fiscalSign = data->fiscalSign;
}

Config4Event::Config4Event() : memory(NULL) {

}

void Config4Event::bind(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
//	memset(&data, 0, sizeof(data));
}

MemoryResult Config4Event::init(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	memset(&data, 0, sizeof(data));
	return save();
}

MemoryResult Config4Event::load(Memory *memory) {
	this->memory = memory;
	this->address = memory->getAddress();
	MemoryCrc crc(memory);
	return crc.readDataWithCrc(&data, sizeof(data));
}

MemoryResult Config4Event::save() {
	if(memory == NULL) {
		LOG_ERROR(LOG_CFG, "Memory not inited");
		return MemoryResult_WrongData;
	}
	memory->setAddress(address);
	MemoryCrc crc(memory);
	return crc.writeDataWithCrc(&data, sizeof(data));
}

void Config4Event::setId(uint32_t id) {
	data.id = id;
}

uint32_t Config4Event::getId() {
	return data.id;
}

void Config4Event::setBusy(uint8_t busy) {
	data.busy = busy;
}

uint8_t Config4Event::getBusy() {
	return data.busy;
}

void Config4Event::set(DateTime *datetime, uint16_t code, const char *str) {
	data.date.set(datetime);
	data.code = code;
	data.data.string.set(str);
}

void Config4Event::set(DateTime *datetime, Fiscal::Sale *sale, uint16_t index) {
	data.date.set(datetime);
	data.code = Type_Sale;

	Fiscal::Product *product = sale->products.get(index);
	data.sale.selectId.set(product->selectId.get());
	data.sale.wareId = product->wareId;
	data.sale.name.set(product->name.get());
	data.sale.price = product->price;
	data.sale.taxRate = product->taxRate;

	data.sale.device.set(sale->device.get());
	data.sale.priceList = sale->priceList;
	data.sale.taxSystem = sale->taxSystem;
	data.sale.taxValue = sale->taxValue;
	data.sale.loyalityType = sale->loyalityType;
	data.sale.loyalityCode.set(sale->loyalityCode.getData(), sale->loyalityCode.getLen());
	data.sale.fiscalRegister = sale->fiscalRegister;
	data.sale.fiscalStorage = sale->fiscalStorage;
	data.sale.fiscalDocument = sale->fiscalDocument;
	data.sale.fiscalSign = sale->fiscalSign;
}

void Config4Event::set(Config4EventStruct *entry) {
	data.date.set(&entry->date);
	data.code = entry->code;
	if(data.code == Type_Sale) {
		data.sale.set(&entry->sale);
	} else {
		data.data.string.set(entry->data.string.get());
	}
}

uint32_t Config4Event::getDataSize() {
	return (sizeof(Config4EventStruct));
}

const char *Config4Event::getEventName(Config4Event *event) {
	switch(event->getCode()) {
	case Type_OnlineStart: return "Связь установлена";
	case Type_OnlineEnd: return "Связь потеряна";
	case Type_OnlineLast: return "Обмен данными";
	case Type_Sale: return "Продажа";
	case Type_PaymentBlocked: return "Продажи отключены";
	case Type_PaymentUnblocked: return "Продажи включены";
	case Type_PowerUp: return "Автомат включен";
	case Type_PowerDown: return "Автомат выключен";
	case Type_BadSignal: return "Плохой сигнал";
	case Type_CashlessIdNotFound: return "Ошибка настройки";
	case Type_PriceListNotFound: return "Ошибка настройки";
	case Type_SyncConfigError: return "Ошибка настройки";
	case Type_PriceNotEqual: return "Ошибка настройки";
	case Type_SaleDisabled: return "Продажи заблокированы";
	case Type_SaleEnabled: return "Продажи включены";
	case Type_ConfigEdited: return "Конфигурация обновлена";
	case Type_ConfigLoaded: return "Конфигурация обновлена";
	case Type_ConfigLoadFailed: return "Ошибка конфигурации";
	case Type_ConfigParseFailed: return "Ошибка конфигурации";
	case Type_ConfigUnknowProduct: return "Ошибка конфигурации";
	case Type_ConfigUnknowPriceList: return "Ошибка конфигурации";
	case Type_FiscalUnknownError: return "Ошибка ФР";
	case Type_FiscalLogicError: return "Ошибка ФР";
	case Type_FiscalConnectError: return "Ошибка ФР";
	case Type_FiscalPassword: return "Ошибка ФР";
	case Type_PrinterNotFound: return "Ошибка принтера";
	case Type_PrinterNoPaper: return "Ошибка принтера";
	case Type_EventReadError: return "Ошибка модема";
	case Type_ModemReboot: return "Модем перезагружен";
	case Type_CashCanceled: return "Продажа";
	case Type_SaleFailed: return "Продажа";
	case Type_BillIn: return "Наличные";
	case Type_CoinIn: return "Наличные";
	case Type_ChangeOut: return "Наличные";
	case Type_CashlessCanceled: return "Наличные";
	case Type_CashlessDenied: return "Наличные";
	default: return "Unknown";
	}
}

void Config4Event::getEventDescription(Config4Event *event, StringBuilder *buf) {
	buf->clear();
	switch(event->getCode()) {
	case Type_Sale: getEventSaleDescription(event, buf); return;
	case Type_CashlessIdNotFound: *buf << "Продукта с номером " << event->getString() << " нет в планограмме"; return;
	case Type_PriceListNotFound: *buf << "Прайс-листа " << event->getString() << " нет в планограмме"; return;
	case Type_SyncConfigError: *buf << "Планограммы не совпадают"; return;
	case Type_PriceNotEqual: getEventPriceNotEqualDescription(event, buf); return;
	case Type_ConfigEdited: *buf << "Конфигурация изменена локально"; return;
	case Type_ConfigLoaded: *buf << "Конфигурация загружена с сервера"; return;
	case Type_ConfigLoadFailed: *buf << "Ошибка загрузки конфигурации"; return;
	case Type_ConfigParseFailed: *buf << "Ошибка формата конфигурации"; return;
	case Type_ConfigUnknowProduct: *buf << "Неожиданный номер продукта " << event->getString(); return;
	case Type_ConfigUnknowPriceList: *buf << "Неожиданный прайс-лист" << event->getString(); return;
	case Type_FiscalUnknownError: *buf << "Код ошибки " << event->getString(); return;
	case Type_FiscalLogicError: *buf << "Ошибка протокола " << event->getString(); return;
	case Type_FiscalConnectError: *buf << "Нет связи с ФР"; return;
	case Type_FiscalPassword: *buf << "Неправильный пароль ФР"; return;
	case Type_PrinterNotFound: *buf << "Принтер не найден"; return;
	case Type_PrinterNoPaper: *buf << "В принтере закончилась бумага"; return;
	case Type_EventReadError: *buf << "Ошибка чтения очереди событий"; return;
	case Type_ModemReboot: *buf << "Перезапуск модема"; return;
	case Type_CashCanceled: *buf << "Оплата наличными отменена автоматом"; return;
	case Type_SaleFailed: *buf << "Ошибка продажи"; return; // (STRING:<selectId>)
	case Type_WaterOverflow: *buf << "Переполнение ведра жидких отходов"; return;
	case Type_FiscalNotInited: *buf << "ККТ не фискализировано"; return;
	case Type_WrongResponse: *buf << "Некорректный формат ответа"; return;
	case Type_BrokenResponse: *buf << "Поврежденный ответ"; return;
	case Type_FiscalCompleteNoData: *buf << "Чек создан, но реквизиты не получены"; return;
	case Type_BillIn: *buf << "Принята купюра"; return; // (STRING:<nominal>)
	case Type_BillUnwaitedPacket: *buf << "Ошибка купюроприемника"; return;
	case Type_CoinIn: *buf << "Принята монета"; return; // (STRING:<nominal>)
	case Type_ChangeOut: *buf << "Выдана сдача"; return; // (STRING:<sum>)
	case Type_CoinUnwaitedPacket: *buf << "Ошибка монетопремник"; return; // (неожиданный пакет)
	case Type_CashlessCanceled: *buf << "Безналичная оплата отменена автоматом"; return;
	case Type_CashlessDenied: *buf << "Запрет оплаты платежным терминалом"; return; // (STRING:<selectId>)
	default:;
	}
}

void Config4Event::getEventSaleDescription(Config4Event *event, StringBuilder *buf) {
	Config4EventSale *sale = event->getSale();
	*buf << "\"" << sale->name.get() << "\" за " << sale->price << paymentDeviceToString(sale->device.get());
}

void Config4Event::getEventPriceNotEqualDescription(Config4Event *event, StringBuilder *buf) {
	const char *def = "Цена не совпадает с планограммой";
	StringParser parser(event->getString());
	buf->clear();
	char selectId[8];
	if(parser.getValue("*", selectId, sizeof(selectId)) == 0) {
		*buf << def;
		return;
	}
	if(parser.compareAndSkip("*") == false) {
		*buf << def;
		return;
	}
	uint32_t expPrice = 0;
	if(parser.getNumber<uint32_t>(&expPrice) == false) {
		*buf << def;
		return;
	}
	if(parser.compareAndSkip("*") == false) {
		*buf << def;
		return;
	}
	uint32_t actPrice = 0;
	if(parser.getNumber<uint32_t>(&actPrice) == false) {
		*buf << def;
		return;
	}
	*buf << "Цена не совпадает с планограммой (кнопка " << selectId << ", планограмма " << expPrice << ", автомат " << actPrice << ")";
}

const char *Config4Event::paymentDeviceToString(const char *device) {
	if(strcasecmp("CA", device) == 0) {
		return " наличными";
	}
	if(strcasecmp("DA", device) == 0 || strcasecmp("DB", device) == 0) {
		return " электронными";
	}
	return "";
}
