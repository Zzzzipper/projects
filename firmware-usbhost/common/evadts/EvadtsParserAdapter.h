#ifndef COMMON_EVADTS_PARSERADAPTER_H_
#define COMMON_EVADTS_PARSERADAPTER_H_

#include "evadts/EvadtsParser.h"
#include "dex/include/DexDataParser.h"
#include "utils/include/Event.h"

class EvadtsParserAdapter : public Dex::DataParser {
public:
	EvadtsParserAdapter(Evadts::Parser *parser, EventObserver *observer = NULL) : parser(parser), courier(observer) {

	}
	virtual ~EvadtsParserAdapter() {
		delete parser;
	}
	virtual void setObserver(EventObserver *observer) {
		courier.setRecipient(observer);
	}
	virtual Result start(uint32_t dataSize) {
		(void)dataSize;
		parser->start();
		return Result_Ok;
	}
	virtual Result procData(const uint8_t *data, const uint16_t len) {
		parser->procData(data, len);
		return Result_Ok;
	}
	virtual Result complete() {
		parser->complete();
		courier.deliver(Event_AsyncOk);
		return Result_Ok;
	}
	virtual void error() {
		courier.deliver(Event_AsyncError);
	}

private:
	Evadts::Parser *parser;
	EventCourier courier;
};

#endif
