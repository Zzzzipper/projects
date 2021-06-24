#ifndef LIB_CLIENT_DEVICENEFTEMAG_H
#define LIB_CLIENT_DEVICENEFTEMAG_H

#include "ClientContext.h"

#include "common/code_scanner/CodeScanner.h"

class ClientDeviceNeftemag : public CodeScanner::Observer {
public:
	ClientDeviceNeftemag(ClientContext *context, CodeScannerInterface *scanner);
	bool procCode(uint8_t *data, uint16_t dataLen) override;

private:
	ClientContext *context;
};

#endif
