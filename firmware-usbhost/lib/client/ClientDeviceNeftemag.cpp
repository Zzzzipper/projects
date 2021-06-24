#include "ClientDeviceNeftemag.h"

#include "common/utils/include/StringParser.h"
#include "common/utils/include/CodePage.h"

ClientDeviceNeftemag::ClientDeviceNeftemag(
	ClientContext *context,
	CodeScannerInterface *scanner
) :
	context(context)
{
	scanner->addObserver(this);
}

bool ClientDeviceNeftemag::procCode(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_ORDERP, "procCode");
	StringParser parser((char*)data, dataLen);
	if(parser.compareAndSkip("10040") == false) {
		return false;
	}

	uint32_t loyalityLen = convertBase64ToBin((uint8_t*)parser.unparsed(), parser.unparsedLen(), data, dataLen);
	LOG_DEBUG(LOG_ORDERP, "Loyality code " << parser.unparsedLen() << "/" << loyalityLen);
	LOG_DEBUG_HEX(LOG_ORDERP, data, loyalityLen);
	if(context->reset() == false) {
		return true;
	}

	context->setLoyality(Loyality_Nefm, data, loyalityLen);
	return true;
}
