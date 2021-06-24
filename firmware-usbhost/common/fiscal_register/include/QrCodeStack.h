#ifndef COMMON_FISCALREGISTER_QRCODESTACK_H
#define COMMON_FISCALREGISTER_QRCODESTACK_H

#include "common/utils/include/List.h"

#define QR_HEADER_SIZE 16
#define QR_FOOTER_SIZE 50
#define QR_TEXT_SIZE 120

class QrCodeInterface {
public:
	virtual ~QrCodeInterface() {}
	virtual bool drawQrCode(const char *header, const char *footer, const char *text) = 0;
};

class QrCodeStack : public QrCodeInterface {
public:
	void push(QrCodeInterface *masterCashless);
	bool drawQrCode(const char *header, const char *footer, const char *text) override;

private:
	List<QrCodeInterface> list;
};

class ScreenInterface : public QrCodeInterface {
public:
	virtual ~ScreenInterface() {}
	virtual bool drawText(const char *text, uint8_t fontMultipleSize = 3, uint16_t textClr = 0xFFFF, uint16_t backgroundClr = 0) = 0;
	virtual bool drawProgress(const char *text, uint8_t fontMultipleSize = 3, uint16_t textClr = 0xFFFF, uint16_t backgroundClr = 0) = 0;
	virtual bool drawImage() = 0;
	virtual bool clear() = 0;
};

#endif
