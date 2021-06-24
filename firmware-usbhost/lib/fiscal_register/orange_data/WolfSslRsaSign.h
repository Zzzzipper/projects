#ifndef LIB_ORANGEDATA_WOLFSSLRSASIGN_H
#define LIB_ORANGEDATA_WOLFSSLRSASIGN_H

#include "config/include/ConfigModem.h"

#include <stdint.h>

class RsaSignInterface {
public:
	virtual ~RsaSignInterface() {}
	virtual bool init(uint8_t *pemPrivateKey, uint16_t pemPrivateKeyLen) = 0;
	virtual bool sign(uint8_t *data, uint16_t dataLen) = 0;
	virtual uint16_t base64encode(uint8_t *buf, uint16_t bufSize) = 0;
};

struct RsaKey;

class WolfSslRsaSign : public RsaSignInterface {
public:
	WolfSslRsaSign();
	virtual ~WolfSslRsaSign();
	virtual bool init(ConfigCert *cert);
	virtual bool init(uint8_t *pemPrivateKey, uint16_t pemPrivateKeyLen);
	virtual bool sign(uint8_t *data, uint16_t dataLen);
	virtual uint16_t base64encode(uint8_t *buf, uint16_t bufSize);

private:
	RsaKey *key;
	uint8_t signBuf[256];
};

#endif
