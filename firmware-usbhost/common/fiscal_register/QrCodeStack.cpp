#include "include/QrCodeStack.h"

void QrCodeStack::push(QrCodeInterface *printer) {
	if(printer != NULL) {
		list.add(printer);
	}
}

bool QrCodeStack::drawQrCode(const char *header, const char *footer, const char *text) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		QrCodeInterface *printer = list.get(i);
		printer->drawQrCode(header, footer, text);
	}
	return true;
}
