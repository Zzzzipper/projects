#ifndef COMMON_CONFIG_V4_EVENT_H_
#define COMMON_CONFIG_V4_EVENT_H_

#include "Config4EventData.h"
#include "fiscal_register/include/FiscalSale.h"
#include "memory/include/Memory.h"

class Config4Event {
public:
	enum Code : uint16_t {
		Type_None					= 0xFFFF,
		Type_OnlineStart			= 0x0000, // Связь установлена
		Type_OnlineEnd				= 0x0001, // Связь потеряна
		Type_OnlineLast				= 0x0002, // Обмен данными
		Type_Sale					= 0x0003, // Продажа
		Type_PaymentBlocked			= 0x0004, // Продажи отключены (присылается, если платежи заблокированы слишком долго)
		Type_PaymentUnblocked		= 0x0005, // Продажи включены
		Type_PowerUp				= 0x0006, // Автомат включен
		Type_PowerDown				= 0x0007, // Автомат выключен
		Type_BadSignal				= 0x0008, // Плохой сигнал
		Type_CashlessIdNotFound		= 0x0009, // ТА запросил некорректный номер продукта (STRING:<cashlessId>)
		Type_PriceListNotFound		= 0x000A, // Прайс лист не найден (STRING:<deviceId><priceListNumber>)
		Type_SyncConfigError		= 0x000B, // Не используется (обратная совместимость)
		Type_PriceNotEqual			= 0x000C, // ТА запросил некорректную цену продукта (STRING:<selectId>*<expectedPrice>*<actualPrice>)
		Type_SaleDisabled			= 0x000D, // Продажи отключены автоматом длительное время
		Type_SaleEnabled			= 0x000E, // Продажи включены автоматом
		Type_ConfigEdited			= 0x000F, // Конфигурация изменена локально
		Type_ConfigLoaded			= 0x0010, // Конфигурация загружена с сервера
		Type_ConfigLoadFailed		= 0x0011, // Ошибка загрузки конфигурации
		Type_ConfigParseFailed		= 0x0012, // Ошибка разбора конфигурации
		Type_ConfigUnknowProduct	= 0x0013, // Неожиданный номер продукта (STRING:<selectId>)
		Type_ConfigUnknowPriceList	= 0x0014, // Неожиданный прайс-лист (STRING:<deviceId><priceListNumber>)
		Type_ModemReboot			= 0x0015, // Перезапуск модема (STRING:<rebootReason>)
		Type_CashCanceled			= 0x0016, // Оплата наличными отменена автоматом
		Type_SaleFailed				= 0x0017, // Ошибка продажи (STRING:<selectId>)
		Type_WaterOverflow			= 0x0018, // Переполнение ведра жидких отходов
		Type_FiscalUnknownError		= 0x0300, // Необрабатываемая ошибка ФР (STRING:<код-ошибки-ФР>)
		Type_FiscalLogicError		= 0x0301, // Неожиданное поведение ФР (STRING:<строка-в-файле>)
		Type_FiscalConnectError		= 0x0302, // Нет связи с ФР
		Type_FiscalPassword			= 0x0303, // Неправильный пароль ФР
		Type_PrinterNotFound		= 0x0304, // Принтер не найден
		Type_PrinterNoPaper			= 0x0305, // В принтере закончилась бумага
		Type_FiscalNotInited		= 0x0306, // ККТ не фискализировано
		Type_WrongResponse			= 0x0307, // Некорректный формат ответа
		Type_BrokenResponse			= 0x0308, // Поврежденный ответ
		Type_FiscalCompleteNoData	= 0x0309, // Чек создан, но реквизиты не получены
		Type_BillIn					= 0x0401, // Принята купюра (STRING:<nominal>)
		Type_BillUnwaitedPacket		= 0x0402, // Ошибка купюроприемника (неожиданный пакет)
		Type_CoinIn					= 0x0501, // Принята монета (STRING:<nominal>)
		Type_ChangeOut				= 0x0502, // Выдана сдача (STRING:<sum>)
		Type_CoinUnwaitedPacket		= 0x0503, // Ошибка монетопремник (неожиданный пакет)
		Type_CashlessCanceled		= 0x0601, // Безналичная оплата отменена автоматом (STRING:<selectId><timeout>)
		Type_CashlessDenied			= 0x0602, // Запрет оплаты платежным терминалом (STRING:<selectId><timeout>)
		Type_SessionClosedByMaster	= 0x0603, // Ожидание безналичной оплаты прервано автоматом (STRING:<timeout>)
		Type_SessionClosedByTimeout	= 0x0604, // Ожидание безналичной оплаты прервано по таймауту (STRING:<timeout>)
		Type_SessionClosedByTerminal= 0x0605, // Ожидание безналичной оплаты прервано терминалом (STRING:<timeout>)
		Type_EventReadError			= 0xFF01, // Ошибка чтения очереди событий
		Type_WatchDog				= 0xFF02, // Сработал WatchDog
		Type_MdbUnwaitedPacket		= 0xFF03, // Неожиданный пакет в MDB-протоколе
		Type_DtsUnwaitedEvent		= 0xFF04,
	};

	Config4Event();
	void bind(Memory *memory);
	MemoryResult init(Memory *memory);
	MemoryResult load(Memory *memory);
	MemoryResult save();
	void setId(uint32_t id);
	uint32_t getId();
	void setBusy(uint8_t busy);
	uint8_t getBusy();

	void set(DateTime *datetime, uint16_t code, const char *str = "");
	void set(DateTime *datetime, Fiscal::Sale *data, uint16_t index);
	void set(Config4EventStruct *data);
	DateTime *getDate() { return &data.date; }
	uint16_t getCode() { return data.code; }
	const char *getString() { return data.data.string.get(); }
	Config4EventSale *getSale() { return &(data.sale); }
	Config4EventStruct *getData() { return &data; }

	static uint32_t getDataSize();
	static const char *getEventName(Config4Event *event);
	static void getEventDescription(Config4Event *event, StringBuilder *buf);

private:
	Memory *memory;
	uint32_t address;
	Config4EventStruct data;

	static void getEventSaleDescription(Config4Event *event, StringBuilder *buf);
	static void getEventPriceNotEqualDescription(Config4Event *event, StringBuilder *buf);
	static const char *paymentDeviceToString(const char *device);
};

#endif
