#include "WolfSslRsaSign.h"

#include "logger/include/Logger.h"

#include "wolfssl/wolfcrypt/asn.h"
#include "wolfssl/wolfcrypt/rsa.h"
#include "wolfssl/wolfcrypt/coding.h"

WolfSslRsaSign::WolfSslRsaSign() : key(NULL) {
}

WolfSslRsaSign::~WolfSslRsaSign() {
	if(key != NULL) {
		wc_FreeRsaKey(key);
		delete key;
	}
}

bool WolfSslRsaSign::init(ConfigCert *cert) {
	StringBuilder buf(CERT_SIZE, CERT_SIZE);
	MemoryResult result = cert->load(&buf);
	if(result != MemoryResult_Ok) {
		return false;
	}
	return init(buf.getData(), buf.getLen());
}

bool WolfSslRsaSign::init(uint8_t *pemPrivateKey, uint16_t pemPrivateKeyLen) {
	LOG_DEBUG(LOG_FRP, "init");
	uint8_t derBuf[1500];
	int derBufSize = sizeof(derBuf);
	int ret = wc_KeyPemToDer(pemPrivateKey, pemPrivateKeyLen, derBuf, derBufSize, 0);
	if(ret <= 0) {
		LOG_ERROR(LOG_FRP, "wc_KeyPemToDer failed");
		return false;
	}
	uint32_t derLen = ret;
	word32 keyOffset = 0;
	int offset = wc_GetPkcs8TraditionalOffset(derBuf, &keyOffset, derLen);
	if(offset <= 0) {
		LOG_ERROR(LOG_FRP, "wc_GetPkcs8TraditionalOffset failed " << offset);
		return false;
	}

	key = new RsaKey;
	if(key == NULL) {
		LOG_ERROR(LOG_FRP, "new failed");
		return false;
	}
	if(wc_InitRsaKey(key, 0) != 0) {
		LOG_ERROR(LOG_FRP, "wc_InitRsaKeyKey failed");
		delete key;
		key = NULL;
		return false;
	}
	if(wc_RsaPrivateKeyDecode(derBuf, &keyOffset, key, derLen) != 0) {
		LOG_ERROR(LOG_FRP, "wc_RsaPrivateKeyDecode failed");
		wc_FreeRsaKey(key);
		delete key;
		key = NULL;
		return false;
	}
	return true;
}
/*
wrong RSA key
x30;x82;x04;xA3;x02;x01;x00;x02;x82;x01;x01;x00;x9E;x13;x31;x6B;xF2;x22;xBA;x7B;xAF;xC9;x14;x73;xB9;x68;x3E;xE0;x1B;xC0;xC3;x9A;xA2;xC6;x7D;x51;x55;x8E;xC4;xF4;x84;xAE;x1E;xE8;x7F;x83;xAD;x62;x95;xE8;x24;xF0;x92;x66;x65;xA5;x83;x11;x75;x1A;xF5;x56;x82;x16;xBA;xA2;xA6;xC3;xAA;x94;x5E;x1A;x3D;x1E;xB3;xA8;x54;x81;xD9;xB8;xCD;x97;xCA;xAD;x8C;x7B;x91;xEF;x51;x94;x4F;x13;x86;xD6;x09;xCE;xE2;xBF;x39;x6A;x93;xC4;xEA;x53;x6E;x54;x83;x61;x73;x9A;x0C;x47;x78;xDC;x51;xAD;x75;x9D;x30;xAA;xD7;x9C;x45;x76;xC4;x05;x51;x53;x31;xD4;x69;x27;xB4;xF5;x7C;xCA;xB9;xED;x99;xAD;x61;xB4;xB5;xBD;x6E;x42;xC8;xA4;x79;x66;x26;x34;x64;xF3;x79;x02;x2F;x7D;x36;x98;x41;x63;x7D;x3A;xDE;x69;xB0;x35;x4A;xEF;x6B;x45;xAE;xC9;x1B;xC2;xBE;x80;x7A;xCC;xB1;x97;x01;x2F;xEB;xDB;x3E;xD2;x09;x91;xC4;x16;x42;xFB;x2A;x6A;xA0;x17;x2C;x20;xDF;x72;xD5;x3B;xE4;xB4;x90;x91;xCB;x9E;x4F;x69;xA7;x35;x4E;x35;xD3;x8D;x63;xD8;x20;xA6;x3E;x20;x87;x11;x46;x5C;x61;x94;xAB;x91;x10;x67;x74;x77;x76;x69;x14;xE6;x87;xD6;xFC;xD9;xC5;xE9;x34;x57;x37;xD9;xFC;xD2;x5C;x66;x9F;x23;xC7;xF0;xE8;xD1;x72;x1C;x59;x70;x9D;x78;x75;xD7;x02;x03;x01;x00;x01;x02;x82;x01;x00;x26;x34;xBD;x5D;x39;xC8;xEB;x09;xBA;x12;xFF;xBE;x78;xB1;x99;x80;xD1;x34;x66;x12;x9A;x05;x1F;x84;xE9;x08;xF8;xD1;xA0;xBF;xF4;xF8;x7C;xD9;x76;xAE;xBD;x4C;xBE;xA0;xE6;xC1;x99;x0F;xC7;xFE;x10;x2F;xD3;xDC;x74;xD4;xC9;x87;x46;x87;x44;x53;x57;xEB;x3F;x9A;x8C;x11;xC3;x5B;x1F;x24;x6B;x8C;xA4;x90;x0D;xC4;x50;x21;x67;xB6;x50;xE6;x87;x50;x74;x4E;x6C;xBF;x8B;x41;x17;x36;x20;xEC;xC9;x5D;xE3;xE3;x49;xED;x91;xCA;x34;x8C;x0B;xB1;xC1;x1B;x9D;x07;xAD;xD5;x37;x74;xDD;x2B;xEB;xDC;x0F;xCF;x27;x1B;x51;xC1;x83;xC7;xFB;xE8;x51;x9C;x15;x12;xD9;xD5;xCE;x37;xDE;xCB;x73;x29;xB8;x65;x48;xBC;x64;x1D;x64;x55;x88;xD4;x72;x3F;x35;x65;x44;x41;x8A;xAA;x31;x17;x03;xEE;xFE;xF0;x3F;xB2;x58;x02;x7B;x17;x11;xBD;xB7;xE6;x2B;x62;x9D;xBC;x2A;x20;xF4;x14;x46;xC0;x82;x48;xEB;xE5;xF7;x50;x5A;xF8;xD4;x73;x4A;x9E;x2C;x10;x2B;xE0;x77;x8F;x12;x6F;xEF;x09;x39;xF0;xA6;x4F;x05;x88;x41;x86;xB3;xA7;x8A;xBC;x15;xEE;x15;x41;xFE;x39;x77;xAD;x94;x49;x8E;x17;x7E;xC5;x2E;xA5;xA1;xA5;x24;xA6;x10;x3D;x41;x66;xC8;x38;x01;x92;x1E;xCC;x8B;x7E;xE9;xDA;x80;xB0;x59;x33;xDD;x01;x02;x81;x81;x00;xD0;x9F;xA1;x7B;xE7;x41;xF1;x40;x3F;xF9;x18;x43;xE3;x0C;x5A;x61;xB7;x16;xBF;x7F;xC2;x97;xF2;x0E;xCE;x64;xBC;xE1;x17;x4E;xF5;x28;x82;xF4;x94;x0B;x56;x1B;x9C;xB9;x2E;xC9;xFA;x18;x4D;x1A;xAF;xED;x02;x31;xFD;x9B;xF7;xF6;x42;xFA;x1F;x32;xB6;x7D;x2F;xA8;x69;x11;x29;xD6;x76;x55;xE9;x16;x35;xD2;xB2;x59;xF6;xDF;x13;x0F;x25;xD5;xF9;x77;xB6;xC6;x4D;x4B;xB7;x51;x6B;x79;xA1;x87;x9F;xBD;x2C;xA2;xAA;xA9;x77;x39;x11;x61;x5F;xF9;x3B;x85;x33;x9B;x98;x10;x05;x0D;x86;x77;xDD;xDE;x6A;x3C;x07;xA7;xCB;xF7;x47;xAC;x7E;x08;xC9;x53;x02;x81;x81;x00;xC1;xF8;xE9;x30;xD9;x03;x37;x88;x22;xD8;x70;x7A;x0E;x8B;xE3;xC5;x0D;x8E;x97;x84;x25;x86;x1E;x7E;x65;x3E;x1D;x8C;x1F;xF4;x6B;xAC;x61;xA0;xD5;xC4;x78;x9B;x61;x06;x5F;xD0;x7D;xFA;x9A;xFD;x6F;xE2;x50;xDB;xEC;xEC;x27;x83;xFB;x74;xED;xC6;x69;x62;x3A;x3E;x89;x73;x26;x28;x35;x69;x86;xB9;x2C;xFD;xB6;x4B;x8B;x2A;xA5;xB2;xFD;x5E;x04;xAC;x16;x5E;x05;x4E;xA2;xA2;xFF;xC4;x7D;x9F;x9F;x48;xB2;xC5;x9A;xC2;xAE;x35;xC9;xC7;x1B;xED;x34;xE2;x17;x4E;xAE;x37;x52;xB9;xD1;xD0;x99;xCD;x60;x50;x61;xC0;xD5;xD4;xBC;xC6;x4A;x4B;x1C;xED;x02;x81;x80;x09;x49;x21;xA1;xE7;x30;x2B;x31;xC5;xE7;x2B;x6A;x52;x1F;xC9;xA2;x11;xC9;x24;x6A;xE6;x59;x66;xFF;xBB;xB6;x06;x26;x2A;xA8;x6C;x07;x0A;x95;x22;x45;xC2;xE6;x65;xBA;x64;x57;xBF;x16;xF7;xCF;x99;x46;xFE;x53;x05;x4B;xBC;xE4;xAC;x24;x7F;xE0;xFC;xF8;x63;x77;xA6;x7D;x8A;x14;x2E;x9E;x32;x4A;xB7;xC8;x92;x01;xA9;x18;x52;xBB;xD8;xDE;x46;x45;x4A;xD0;x56;xBE;x69;x01;x69;xBC;x37;x31;x57;x2D;xCF;x5F;xCA;x22;xD2;xD8;xAA;x6A;x60;x20;x32;xC3;x02;x02;x09;xE9;xA4;x9F;xEE;x7D;x45;x55;xD1;xFA;xAC;x08;x8E;xEE;x89;xB5;x26;xC2;xF3;x5B;x02;x81;x80;x55;x6E;xCD;x3E;x5F;x86;xA4;x31;xC8;xAF;x2D;xB3;x04;xAF;x26;x6D;xC7;x0F;xD3;xBA;x37;x50;xC0;x23;x89;x71;xF7;x4F;x9D;x4F;x69;x11;x2E;x9A;xC9;x2C;x54;xB6;x92;xE0;x5C;xD9;x16;x73;x87;x4A;x32;xBA;x2A;x45;x03;x2F;xEC;x23;x9C;x60;x1E;xCD;xF1;xE2;x7A;xA6;xCA;xA1;x35;xE0;x25;xCE;x49;xB1;x4B;x98;x9F;x6E;xDC;x67;xB7;x3D;x83;x8C;xA2;x60;x2D;x25;xD2;x0E;x95;x61;x57;x67;x72;xB1;x18;x55;xCB;xAF;x6E;xFF;x69;xFD;x74;xD6;xC6;x7D;x0E;x7A;xBA;x2B;x44;x5E;x47;x88;x62;x5A;x0F;x33;xBE;xC5;x08;x99;x07;x55;x40;xA3;xD1;x5B;x0D;xED;x02;x81;x81;x00;x82;x9F;x1B;xF2;x2E;x2F;x30;xD6;x88;x2B;xC9;x60;x77;x75;x61;x83;x6F;x39;x87;x5D;x6F;x5A;xA8;x77;x5D;xB8;x03;xBD;xE1;x0C;x4C;xA3;x1D;x0E;x6F;x6D;xBE;x6D;xA1;x64;x1F;x95;x70;x52;x01;x50;x76;x0E;x7D;x41;x80;x83;xA2;xBD;xDC;xE9;x5D;x5D;xC6;x3D;x46;xAF;xDB;x6F;x1E;xE5;x52;x4A;x4B;x5B;x26;x37;xA8;xB8;x78;xA5;x7E;x2F;x07;x45;x5A;x86;xB7;x53;x31;x91;x21;xD7;x90;x0A;x6D;x17;x2B;x6C;xC7;x49;xDB;xA5;x8C;x4B;xC0;x9B;xE9;xAB;x97;x18;x74;xF9;x71;x85;xE6;xEC;xCC;x3B;x5B;x03;x06;xDF;x0D;xCB;x34;x90;x88;x8D;xB6;x4C;xB7;xA5;

FIELD(x30;)LEN(x82;x04;xA3;)VALUE(all)
  FIELD(x02;)LEN(x01;)VALUE(x00;)
  FIELD(x02;)LEN(x82;x01;x01;)VALUE(x00;x9E;x13;x31;x6B;xF2;x22;xBA;x7B;xAF;xC9;x14;x73;xB9;x68;x3E;xE0;x1B;xC0;xC3;x9A;xA2;xC6;x7D;x51;x55;x8E;xC4;xF4;x84;xAE;x1E;xE8;x7F;x83;xAD;x62;x95;xE8;x24;xF0;x92;x66;x65;xA5;x83;x11;x75;x1A;xF5;x56;x82;x16;xBA;xA2;xA6;xC3;xAA;x94;x5E;x1A;x3D;x1E;xB3;xA8;x54;x81;xD9;xB8;xCD;x97;xCA;xAD;x8C;x7B;x91;xEF;x51;x94;x4F;x13;x86;xD6;x09;xCE;xE2;xBF;x39;x6A;x93;xC4;xEA;x53;x6E;x54;x83;x61;x73;x9A;x0C;x47;x78;xDC;x51;xAD;x75;x9D;x30;xAA;xD7;x9C;x45;x76;xC4;x05;x51;x53;x31;xD4;x69;x27;xB4;xF5;x7C;xCA;xB9;xED;x99;xAD;x61;xB4;xB5;xBD;x6E;x42;xC8;xA4;x79;x66;x26;x34;x64;xF3;x79;x02;x2F;x7D;x36;x98;x41;x63;x7D;x3A;xDE;x69;xB0;x35;x4A;xEF;x6B;x45;xAE;xC9;x1B;xC2;xBE;x80;x7A;xCC;xB1;x97;x01;x2F;xEB;xDB;x3E;xD2;x09;x91;xC4;x16;x42;xFB;x2A;x6A;xA0;x17;x2C;x20;xDF;x72;xD5;x3B;xE4;xB4;x90;x91;xCB;x9E;x4F;x69;xA7;x35;x4E;x35;xD3;x8D;x63;xD8;x20;xA6;x3E;x20;x87;x11;x46;x5C;x61;x94;xAB;x91;x10;x67;x74;x77;x76;x69;x14;xE6;x87;xD6;xFC;xD9;xC5;xE9;x34;x57;x37;xD9;xFC;xD2;x5C;x66;x9F;x23;xC7;xF0;xE8;xD1;x72;x1C;x59;x70;x9D;x78;x75;xD7;)
  FIELD(x02;)LEN(x03;)VALUE(x01;x00;x01;)
  FIELD(x02;)LEN(x82;x01;x00;)VALUE(x26;x34;xBD;x5D;x39;xC8;xEB;x09;xBA;x12;xFF;xBE;x78;xB1;x99;x80;xD1;x34;x66;x12;x9A;x05;x1F;x84;xE9;x08;xF8;xD1;xA0;xBF;xF4;xF8;x7C;xD9;x76;xAE;xBD;x4C;xBE;xA0;xE6;xC1;x99;x0F;xC7;xFE;x10;x2F;xD3;xDC;x74;xD4;xC9;x87;x46;x87;x44;x53;x57;xEB;x3F;x9A;x8C;x11;xC3;x5B;x1F;x24;x6B;x8C;xA4;x90;x0D;xC4;x50;x21;x67;xB6;x50;xE6;x87;x50;x74;x4E;x6C;xBF;x8B;x41;x17;x36;x20;xEC;xC9;x5D;xE3;xE3;x49;xED;x91;xCA;x34;x8C;x0B;xB1;xC1;x1B;x9D;x07;xAD;xD5;x37;x74;xDD;x2B;xEB;xDC;x0F;xCF;x27;x1B;x51;xC1;x83;xC7;xFB;xE8;x51;x9C;x15;x12;xD9;xD5;xCE;x37;xDE;xCB;x73;x29;xB8;x65;x48;xBC;x64;x1D;x64;x55;x88;xD4;x72;x3F;x35;x65;x44;x41;x8A;xAA;x31;x17;x03;xEE;xFE;xF0;x3F;xB2;x58;x02;x7B;x17;x11;xBD;xB7;xE6;x2B;x62;x9D;xBC;x2A;x20;xF4;x14;x46;xC0;x82;x48;xEB;xE5;xF7;x50;x5A;xF8;xD4;x73;x4A;x9E;x2C;x10;x2B;xE0;x77;x8F;x12;x6F;xEF;x09;x39;xF0;xA6;x4F;x05;x88;x41;x86;xB3;xA7;x8A;xBC;x15;xEE;x15;x41;xFE;x39;x77;xAD;x94;x49;x8E;x17;x7E;xC5;x2E;xA5;xA1;xA5;x24;xA6;x10;x3D;x41;x66;xC8;x38;x01;x92;x1E;xCC;x8B;x7E;xE9;xDA;x80;xB0;x59;x33;xDD;x01;)
  FIELD(x02;)LEN(x81;x81;)VALUE(x00;xD0;x9F;xA1;x7B;xE7;x41;xF1;x40;x3F;xF9;x18;x43;xE3;x0C;x5A;x61;xB7;x16;xBF;x7F;xC2;x97;xF2;x0E;xCE;x64;xBC;xE1;x17;x4E;xF5;x28;x82;xF4;x94;x0B;x56;x1B;x9C;xB9;x2E;xC9;xFA;x18;x4D;x1A;xAF;xED;x02;x31;xFD;x9B;xF7;xF6;x42;xFA;x1F;x32;xB6;x7D;x2F;xA8;x69;x11;x29;xD6;x76;x55;xE9;x16;x35;xD2;xB2;x59;xF6;xDF;x13;x0F;x25;xD5;xF9;x77;xB6;xC6;x4D;x4B;xB7;x51;x6B;x79;xA1;x87;x9F;xBD;x2C;xA2;xAA;xA9;x77;x39;x11;x61;x5F;xF9;x3B;x85;x33;x9B;x98;x10;x05;x0D;x86;x77;xDD;xDE;x6A;x3C;x07;xA7;xCB;xF7;x47;xAC;x7E;x08;xC9;x53;)
  FIELD(x02;)LEN(x81;x81;)VALUE(x00;xC1;xF8;xE9;x30;xD9;x03;x37;x88;x22;xD8;x70;x7A;x0E;x8B;xE3;xC5;x0D;x8E;x97;x84;x25;x86;x1E;x7E;x65;x3E;x1D;x8C;x1F;xF4;x6B;xAC;x61;xA0;xD5;xC4;x78;x9B;x61;x06;x5F;xD0;x7D;xFA;x9A;xFD;x6F;xE2;x50;xDB;xEC;xEC;x27;x83;xFB;x74;xED;xC6;x69;x62;x3A;x3E;x89;x73;x26;x28;x35;x69;x86;xB9;x2C;xFD;xB6;x4B;x8B;x2A;xA5;xB2;xFD;x5E;x04;xAC;x16;x5E;x05;x4E;xA2;xA2;xFF;xC4;x7D;x9F;x9F;x48;xB2;xC5;x9A;xC2;xAE;x35;xC9;xC7;x1B;xED;x34;xE2;x17;x4E;xAE;x37;x52;xB9;xD1;xD0;x99;xCD;x60;x50;x61;xC0;xD5;xD4;xBC;xC6;x4A;x4B;x1C;xED;)
  FIELD(x02;)LEN(x81;x80;)VALUE(x09;x49;x21;xA1;xE7;x30;x2B;x31;xC5;xE7;x2B;x6A;x52;x1F;xC9;xA2;x11;xC9;x24;x6A;xE6;x59;x66;xFF;xBB;xB6;x06;x26;x2A;xA8;x6C;x07;x0A;x95;x22;x45;xC2;xE6;x65;xBA;x64;x57;xBF;x16;xF7;xCF;x99;x46;xFE;x53;x05;x4B;xBC;xE4;xAC;x24;x7F;xE0;xFC;xF8;x63;x77;xA6;x7D;x8A;x14;x2E;x9E;x32;x4A;xB7;xC8;x92;x01;xA9;x18;x52;xBB;xD8;xDE;x46;x45;x4A;xD0;x56;xBE;x69;x01;x69;xBC;x37;x31;x57;x2D;xCF;x5F;xCA;x22;xD2;xD8;xAA;x6A;x60;x20;x32;xC3;x02;x02;x09;xE9;xA4;x9F;xEE;x7D;x45;x55;xD1;xFA;xAC;x08;x8E;xEE;x89;xB5;x26;xC2;xF3;x5B;)
  FIELD(x02;)LEN(x81;x80;)VALUE(x55;x6E;xCD;x3E;x5F;x86;xA4;x31;xC8;xAF;x2D;xB3;x04;xAF;x26;x6D;xC7;x0F;xD3;xBA;x37;x50;xC0;x23;x89;x71;xF7;x4F;x9D;x4F;x69;x11;x2E;x9A;xC9;x2C;x54;xB6;x92;xE0;x5C;xD9;x16;x73;x87;x4A;x32;xBA;x2A;x45;x03;x2F;xEC;x23;x9C;x60;x1E;xCD;xF1;xE2;x7A;xA6;xCA;xA1;x35;xE0;x25;xCE;x49;xB1;x4B;x98;x9F;x6E;xDC;x67;xB7;x3D;x83;x8C;xA2;x60;x2D;x25;xD2;x0E;x95;x61;x57;x67;x72;xB1;x18;x55;xCB;xAF;x6E;xFF;x69;xFD;x74;xD6;xC6;x7D;x0E;x7A;xBA;x2B;x44;x5E;x47;x88;x62;x5A;x0F;x33;xBE;xC5;x08;x99;x07;x55;x40;xA3;xD1;x5B;x0D;xED;)
  FIELD(x02;)LEN(x81;x81;)VALUE(x00;x82;x9F;x1B;xF2;x2E;x2F;x30;xD6;x88;x2B;xC9;x60;x77;x75;x61;x83;x6F;x39;x87;x5D;x6F;x5A;xA8;x77;x5D;xB8;x03;xBD;xE1;x0C;x4C;xA3;x1D;x0E;x6F;x6D;xBE;x6D;xA1;x64;x1F;x95;x70;x52;x01;x50;x76;x0E;x7D;x41;x80;x83;xA2;xBD;xDC;xE9;x5D;x5D;xC6;x3D;x46;xAF;xDB;x6F;x1E;xE5;x52;x4A;x4B;x5B;x26;x37;xA8;xB8;x78;xA5;x7E;x2F;x07;x45;x5A;x86;xB7;x53;x31;x91;x21;xD7;x90;x0A;x6D;x17;x2B;x6C;xC7;x49;xDB;xA5;x8C;x4B;xC0;x9B;xE9;xAB;x97;x18;x74;xF9;x71;x85;xE6;xEC;xCC;x3B;x5B;x03;x06;xDF;x0D;xCB;x34;x90;x88;x8D;xB6;x4C;xB7;xA5;)


good RSA key
x30;x82;x04;xBD;x02;x01;x00;x30;x0D;x06;x09;x2A;x86;x48;x86;xF7;x0D;x01;x01;x01;x05;x00;x04;x82;x04;xA7;x30;x82;x04;xA3;x02;x01;x00;x02;x82;x01;x01;x00;xC0;xDC;xD3;xE9;xB8;x05;x9F;x8D;xDD;xDB;xCD;x8B;x1A;x80;xB6;x02;xA3;x94;x93;x19;x6B;xFB;x37;x5A;x9A;xEF;xCE;x8C;x7D;xCF;xC2;x6C;x13;x89;xF9;xC3;xCD;x34;x2C;x2C;x1C;x85;xC8;x44;xB1;xA5;xF8;x97;x80;x12;xF2;x30;xB2;x6D;xA3;xD3;xC3;xBB;x1D;xFA;x6F;x0C;xE2;x87;x40;x14;x02;x14;xC5;xBF;xBD;x35;xB8;x11;xB5;xBE;x99;xA8;xD3;xF5;x6A;x66;xCA;xCA;xFD;xFB;x7A;x17;x8D;x2F;xEA;x3B;x76;xE5;x80;x97;xF5;x27;x9C;x71;x8C;x70;x70;x7C;x7E;x40;x2B;x7C;x2D;x83;x8C;xB6;x6D;x5D;x63;x78;x56;x2E;xBB;x32;x60;x73;xEE;xBA;xB8;x47;x3C;x3E;xE3;x19;x59;xB4;xD4;xB7;x11;xBC;xB7;x91;xC3;xC7;x1F;x9C;x8B;x46;xD5;x13;xF4;x5C;x22;xEF;x28;xCC;xF5;xDC;x43;x98;x61;xF8;x15;x18;x71;x3E;x5B;x55;x03;xA1;xED;x58;x20;x8F;x12;xFA;x2A;x59;xE3;x82;x9C;x38;xBE;xE5;x74;x29;x0B;x49;xCC;x19;xE8;xD9;x5E;xC1;x0A;xD5;x6B;x06;xCB;xE4;x3C;x0E;x00;xA2;x4F;x56;x5F;x5A;x98;x45;xED;xDA;x62;xC7;x76;x08;xA2;xBC;xDD;xB6;xBE;x93;x4C;xEA;x93;x12;xF8;xF8;xB7;x2D;xC8;x85;x46;x2D;x30;x09;xA3;x07;xAA;xD2;x02;x53;xFB;x10;xCC;x8E;x87;x27;xE5;x4C;xA2;x19;x32;xFE;x56;x83;xF9;x05;x10;xAF;x02;x03;x01;x00;x01;x02;x82;x01;x00;x12;x9C;xFD;x62;xC0;x1C;xE2;xD2;x39;x48;x3E;x65;x1F;x70;xAE;xA0;x40;x93;x55;x43;x0D;xD5;xD7;xF8;xC9;x9D;x3D;x4C;xF7;xD3;x76;x4B;x21;xF3;x9E;x04;x54;xA8;xA5;x5E;xB8;x7D;xFC;xDF;x0C;x5A;x1D;x4C;xD9;xD2;x7A;x47;x52;xE1;x1B;xFF;x93;x13;x5B;x08;x51;x71;x67;xE7;x6F;xBA;xBE;x9C;xAC;x9D;x1D;xB0;xB2;x8C;x1B;x03;x43;x27;x35;x15;xA9;x68;x34;x48;x35;x0C;xF1;x32;x96;xB0;xBA;x25;x75;x0B;x1F;x47;x0B;x25;x00;x87;x40;xEB;x95;x1F;x9E;xBB;xA8;xB6;xBA;x59;xC8;xD9;xCE;x62;x45;xF8;x56;x30;xFA;x66;x19;x42;xF7;x16;x54;x34;xA3;xCE;xAE;xC5;x19;x87;x3A;x74;x3E;x69;xA1;x68;x1B;xA7;xEB;xA0;xB6;x26;x0E;x50;xD7;xC0;x75;x5F;x68;xAC;x95;x20;x21;xD6;x80;x67;x58;x83;x9F;x2A;x45;xAA;xCE;x67;x0A;x74;xF3;xB0;x15;x4C;xF0;x23;x60;xDB;x72;x18;x71;x36;xD1;x23;x70;xDB;xD8;x1A;x85;x49;x8F;xE6;x8E;xA8;xCA;x52;xFC;x25;xCA;x81;x0F;xB2;xB2;x68;x9E;xC9;xEC;x88;xD9;x0E;xF9;x9F;x48;x15;x27;x08;x98;x95;x3B;xE6;x95;x97;xDB;xDE;x2C;x88;x70;x49;xB9;x6F;x0C;x47;xB1;xB2;x53;x37;x72;x27;xAC;x69;x3A;x2C;xDF;xB1;xF1;x8D;x6F;x31;x44;x52;x95;x81;xA3;x33;xED;xEB;x64;xE1;x02;x81;x81;x00;xF9;x69;x9C;x4D;xB5;xB2;xBB;x9C;x3A;x4D;xB6;xE1;xDC;xB5;x7E;x74;xDA;x01;xE3;x7D;x18;xAC;xA7;xF2;x22;xF6;x34;x57;xE5;x2E;xED;x2B;x0F;xE0;x57;xFF;xAC;x4C;x14;xC9;x66;x17;x0C;x9D;xA0;x3C;xBF;x6E;x9C;xC8;xF9;x81;x41;xA1;x90;xA1;x6F;xC6;xF2;xE3;xB0;x54;x11;xFA;xA5;x33;x97;xF5;x2E;xE1;x32;x2F;x96;xA6;x4C;x7E;xC2;xE4;x81;xEE;x54;x7A;xBC;x00;x36;xE0;x57;xC1;x82;x3A;x58;xB4;x78;xD0;x3D;xED;xEF;x79;x19;x4B;xD0;x03;x55;xA5;x86;x90;xB0;x26;xA2;x1A;x0C;x96;xF8;x89;xAD;x44;x9E;x0E;x0E;xF9;x37;x45;x39;x42;xDA;x7A;x31;x51;x02;x81;x81;x00;xC5;xF4;xDB;x9F;xB4;x78;x2D;xE6;x89;x94;xA1;xD7;x00;xBD;xEF;xEE;x55;x9D;x9E;x5B;x73;xFB;xEA;x0C;x40;x45;x94;x96;xCF;x32;x98;x49;x06;xF3;xB1;x9C;xD0;xE2;xC8;x7D;x51;x8E;xDA;x53;x17;x5B;x3A;x8C;x69;x28;xF8;x0E;x03;x68;x1C;x95;x75;xED;xF6;x28;xA1;x82;x70;xF7;xDF;xDB;xED;x1C;xE1;xE9;xF9;xA4;xDD;x62;x99;x0C;xAC;x44;x94;x71;xAC;xAA;x50;xB3;x80;x64;x5E;xDD;xDC;xCB;x84;x48;x2D;x0A;x17;x42;x1D;xA0;x4B;xF3;x40;x0F;x4D;x8A;x29;x1A;x1D;xB7;x21;x89;x1D;xB4;x8F;xC9;x62;x64;x71;x30;x1A;xC2;xA8;x0D;xC2;x00;x99;x2A;xA1;xFF;x02;x81;x81;x00;x8A;xA1;x1C;x02;xC9;x8A;x95;xE3;x09;x39;x26;xFC;xB5;x9B;x9F;xB5;x3D;x73;xAD;x49;x5C;x0C;xA3;xB0;xDF;xA3;xEF;x86;x27;x5A;x04;xF3;x59;x78;xBE;x10;xDB;x68;xD2;x68;xFB;x38;xB6;x87;x6A;x88;x39;x73;x36;xEC;x32;x5A;x98;xEB;x3F;xA3;xAB;xA8;x6E;x5B;x06;x28;x44;x72;x07;x9E;xFC;xC8;x88;x0D;x1D;xC2;xFB;xBE;x65;x68;x53;xD5;x85;x2E;xBE;x80;x15;xBD;x1C;xC3;x67;xA3;xA4;x49;xE0;x02;x37;xE0;xAF;x7B;x70;x0E;xE4;x73;x92;x24;x38;x57;xAF;xCA;xFE;x4E;x0A;xED;xE5;xAF;x88;x67;xA6;x0A;x2C;xBB;xED;xB0;xB1;xFA;xE5;x0F;xF6;xB5;xD2;x71;x02;x81;x80;x0F;x33;x05;xC9;xF5;x69;x64;xDC;xD6;xA3;x7A;xE9;xAF;x3C;xE6;x37;x3E;x8B;xA3;xA5;x11;xFA;xBD;xB5;xC3;x19;x94;x97;x1F;xC1;x9A;xBF;xC0;xB0;xE5;x6A;x4C;xFF;x9C;xB5;x42;x95;xDD;x5D;x93;xE5;x85;x51;x52;xA1;xBA;xAA;x18;xC8;xDD;xA8;xFC;x2D;x11;x41;x7D;x65;x2E;x97;x59;xB0;xE1;x3B;xDE;x7C;xC0;x96;x50;x09;x4A;x07;x17;x13;x0C;xF2;xCD;x77;x26;x4B;x22;x08;x92;xE0;x26;x1B;xDA;x44;x50;x70;xFD;xE8;x2D;xAE;x29;x26;xBC;x3C;x70;x8F;xB3;x28;x36;x2F;xE1;x7B;x4D;xB4;x97;x75;xB7;x00;x4F;x50;x8A;x3D;x77;xF9;xD6;x73;x85;x4F;x28;xB1;x02;x81;x80;x39;x69;x6D;x5B;x26;x7A;xF4;x8F;x6A;x88;x45;xD9;x5A;xD8;x50;xC4;x53;x76;x10;x86;x6A;x5A;xA4;xF9;x16;xDD;xAC;xCE;x67;x7E;x93;x70;x12;xBA;xFB;x2B;xEA;xF9;x0F;x0B;xA1;xBB;xC0;x6F;x34;xAD;x9C;xAC;x8F;x23;xED;x46;x7B;xAA;x30;x3C;x1C;xCF;xA1;x3D;x64;xC8;xDB;x9A;x2B;xF9;x13;x13;xE9;xAA;x4D;x40;xC6;x0A;xCA;x44;x5E;x96;x8B;x4F;x59;x10;x1E;x6A;xEF;x26;xDF;xC0;x6D;x9A;xEE;x0B;xB9;xD2;x82;x4B;x51;x21;x10;x01;x5C;x18;xC4;x96;xB2;x60;x2A;xA2;x8A;x51;x01;x8B;xF6;x3C;x1B;x29;x9C;x93;x1B;x48;xCA;x8E;x02;x33;x76;x76;xAE;x16;

FIELD(x30;)LEN(x82;x04;xBD;=2byte+x04;xBD;)VALUE(all)
  FIELD(x02;)LEN(x01;)VALUE(x00;)
  FIELD(x30;)LEN(x0D;)VALUE(x06;x09;x2A;x86;x48;x86;xF7;x0D;x01;x01;x01;x05;x00;)
  FIELD(x04;)LEN(x82;x04;xA7;=1191)VALUE(x30;x82;x04;xA3;x02;x01;x00;x02;x82;x01;x01;x00;xC0;xDC;xD3;xE9;xB8;x05;x9F;x8D;xDD;xDB;xCD;x8B;x1A;x80;xB6;x02;xA3;x94;x93;x19;x6B;xFB;x37;x5A;x9A;xEF;xCE;x8C;x7D;xCF;xC2;x6C;x13;x89;xF9;xC3;xCD;x34;x2C;x2C;x1C;x85;xC8;x44;xB1;xA5;xF8;x97;x80;x12;xF2;x30;xB2;x6D;xA3;xD3;xC3;xBB;x1D;xFA;x6F;x0C;xE2;x87;x40;x14;x02;x14;xC5;xBF;xBD;x35;xB8;x11;xB5;xBE;x99;xA8;xD3;xF5;x6A;x66;xCA;xCA;xFD;xFB;x7A;x17;x8D;x2F;xEA;x3B;x76;xE5;x80;x97;xF5;x27;x9C;x71;x8C;x70;x70;x7C;x7E;x40;x2B;x7C;x2D;x83;x8C;xB6;x6D;x5D;x63;x78;x56;x2E;xBB;x32;x60;x73;xEE;xBA;xB8;x47;x3C;x3E;xE3;x19;x59;xB4;xD4;xB7;x11;xBC;xB7;x91;xC3;xC7;x1F;x9C;x8B;x46;xD5;x13;xF4;x5C;x22;xEF;x28;xCC;xF5;xDC;x43;x98;x61;xF8;x15;x18;x71;x3E;x5B;x55;x03;xA1;xED;x58;x20;x8F;x12;xFA;x2A;x59;xE3;x82;x9C;x38;xBE;xE5;x74;x29;x0B;x49;xCC;x19;xE8;xD9;x5E;xC1;x0A;xD5;x6B;x06;xCB;xE4;x3C;x0E;x00;xA2;x4F;x56;x5F;x5A;x98;x45;xED;xDA;x62;xC7;x76;x08;xA2;xBC;xDD;xB6;xBE;x93;x4C;xEA;x93;x12;xF8;xF8;xB7;x2D;xC8;x85;x46;x2D;x30;x09;xA3;x07;xAA;xD2;x02;x53;xFB;x10;xCC;x8E;x87;x27;xE5;x4C;xA2;x19;x32;xFE;x56;x83;xF9;x05;x10;xAF;x02;x03;x01;x00;x01;x02;x82;x01;x00;x12;x9C;xFD;x62;xC0;x1C;xE2;xD2;x39;x48;x3E;x65;x1F;x70;xAE;xA0;x40;x93;x55;x43;x0D;xD5;xD7;xF8;xC9;x9D;x3D;x4C;xF7;xD3;x76;x4B;x21;xF3;x9E;x04;x54;xA8;xA5;x5E;xB8;x7D;xFC;xDF;x0C;x5A;x1D;x4C;xD9;xD2;x7A;x47;x52;xE1;x1B;xFF;x93;x13;x5B;x08;x51;x71;x67;xE7;x6F;xBA;xBE;x9C;xAC;x9D;x1D;xB0;xB2;x8C;x1B;x03;x43;x27;x35;x15;xA9;x68;x34;x48;x35;x0C;xF1;x32;x96;xB0;xBA;x25;x75;x0B;x1F;x47;x0B;x25;x00;x87;x40;xEB;x95;x1F;x9E;xBB;xA8;xB6;xBA;x59;xC8;xD9;xCE;x62;x45;xF8;x56;x30;xFA;x66;x19;x42;xF7;x16;x54;x34;xA3;xCE;xAE;xC5;x19;x87;x3A;x74;x3E;x69;xA1;x68;x1B;xA7;xEB;xA0;xB6;x26;x0E;x50;xD7;xC0;x75;x5F;x68;xAC;x95;x20;x21;xD6;x80;x67;x58;x83;x9F;x2A;x45;xAA;xCE;x67;x0A;x74;xF3;xB0;x15;x4C;xF0;x23;x60;xDB;x72;x18;x71;x36;xD1;x23;x70;xDB;xD8;x1A;x85;x49;x8F;xE6;x8E;xA8;xCA;x52;xFC;x25;xCA;x81;x0F;xB2;xB2;x68;x9E;xC9;xEC;x88;xD9;x0E;xF9;x9F;x48;x15;x27;x08;x98;x95;x3B;xE6;x95;x97;xDB;xDE;x2C;x88;x70;x49;xB9;x6F;x0C;x47;xB1;xB2;x53;x37;x72;x27;xAC;x69;x3A;x2C;xDF;xB1;xF1;x8D;x6F;x31;x44;x52;x95;x81;xA3;x33;xED;xEB;x64;xE1;x02;x81;x81;x00;xF9;x69;x9C;x4D;xB5;xB2;xBB;x9C;x3A;x4D;xB6;xE1;xDC;xB5;x7E;x74;xDA;x01;xE3;x7D;x18;xAC;xA7;xF2;x22;xF6;x34;x57;xE5;x2E;xED;x2B;x0F;xE0;x57;xFF;xAC;x4C;x14;xC9;x66;x17;x0C;x9D;xA0;x3C;xBF;x6E;x9C;xC8;xF9;x81;x41;xA1;x90;xA1;x6F;xC6;xF2;xE3;xB0;x54;x11;xFA;xA5;x33;x97;xF5;x2E;xE1;x32;x2F;x96;xA6;x4C;x7E;xC2;xE4;x81;xEE;x54;x7A;xBC;x00;x36;xE0;x57;xC1;x82;x3A;x58;xB4;x78;xD0;x3D;xED;xEF;x79;x19;x4B;xD0;x03;x55;xA5;x86;x90;xB0;x26;xA2;x1A;x0C;x96;xF8;x89;xAD;x44;x9E;x0E;x0E;xF9;x37;x45;x39;x42;xDA;x7A;x31;x51;x02;x81;x81;x00;xC5;xF4;xDB;x9F;xB4;x78;x2D;xE6;x89;x94;xA1;xD7;x00;xBD;xEF;xEE;x55;x9D;x9E;x5B;x73;xFB;xEA;x0C;x40;x45;x94;x96;xCF;x32;x98;x49;x06;xF3;xB1;x9C;xD0;xE2;xC8;x7D;x51;x8E;xDA;x53;x17;x5B;x3A;x8C;x69;x28;xF8;x0E;x03;x68;x1C;x95;x75;xED;xF6;x28;xA1;x82;x70;xF7;xDF;xDB;xED;x1C;xE1;xE9;xF9;xA4;xDD;x62;x99;x0C;xAC;x44;x94;x71;xAC;xAA;x50;xB3;x80;x64;x5E;xDD;xDC;xCB;x84;x48;x2D;x0A;x17;x42;x1D;xA0;x4B;xF3;x40;x0F;x4D;x8A;x29;x1A;x1D;xB7;x21;x89;x1D;xB4;x8F;xC9;x62;x64;x71;x30;x1A;xC2;xA8;x0D;xC2;x00;x99;x2A;xA1;xFF;x02;x81;x81;x00;x8A;xA1;x1C;x02;xC9;x8A;x95;xE3;x09;x39;x26;xFC;xB5;x9B;x9F;xB5;x3D;x73;xAD;x49;x5C;x0C;xA3;xB0;xDF;xA3;xEF;x86;x27;x5A;x04;xF3;x59;x78;xBE;x10;xDB;x68;xD2;x68;xFB;x38;xB6;x87;x6A;x88;x39;x73;x36;xEC;x32;x5A;x98;xEB;x3F;xA3;xAB;xA8;x6E;x5B;x06;x28;x44;x72;x07;x9E;xFC;xC8;x88;x0D;x1D;xC2;xFB;xBE;x65;x68;x53;xD5;x85;x2E;xBE;x80;x15;xBD;x1C;xC3;x67;xA3;xA4;x49;xE0;x02;x37;xE0;xAF;x7B;x70;x0E;xE4;x73;x92;x24;x38;x57;xAF;xCA;xFE;x4E;x0A;xED;xE5;xAF;x88;x67;xA6;x0A;x2C;xBB;xED;xB0;xB1;xFA;xE5;x0F;xF6;xB5;xD2;x71;x02;x81;x80;x0F;x33;x05;xC9;xF5;x69;x64;xDC;xD6;xA3;x7A;xE9;xAF;x3C;xE6;x37;x3E;x8B;xA3;xA5;x11;xFA;xBD;xB5;xC3;x19;x94;x97;x1F;xC1;x9A;xBF;xC0;xB0;xE5;x6A;x4C;xFF;x9C;xB5;x42;x95;xDD;x5D;x93;xE5;x85;x51;x52;xA1;xBA;xAA;x18;xC8;xDD;xA8;xFC;x2D;x11;x41;x7D;x65;x2E;x97;x59;xB0;xE1;x3B;xDE;x7C;xC0;x96;x50;x09;x4A;x07;x17;x13;x0C;xF2;xCD;x77;x26;x4B;x22;x08;x92;xE0;x26;x1B;xDA;x44;x50;x70;xFD;xE8;x2D;xAE;x29;x26;xBC;x3C;x70;x8F;xB3;x28;x36;x2F;xE1;x7B;x4D;xB4;x97;x75;xB7;x00;x4F;x50;x8A;x3D;x77;xF9;xD6;x73;x85;x4F;x28;xB1;x02;x81;x80;x39;x69;x6D;x5B;x26;x7A;xF4;x8F;x6A;x88;x45;xD9;x5A;xD8;x50;xC4;x53;x76;x10;x86;x6A;x5A;xA4;xF9;x16;xDD;xAC;xCE;x67;x7E;x93;x70;x12;xBA;xFB;x2B;xEA;xF9;x0F;x0B;xA1;xBB;xC0;x6F;x34;xAD;x9C;xAC;x8F;x23;xED;x46;x7B;xAA;x30;x3C;x1C;xCF;xA1;x3D;x64;xC8;xDB;x9A;x2B;xF9;x13;x13;xE9;xAA;x4D;x40;xC6;x0A;xCA;x44;x5E;x96;x8B;x4F;x59;x10;x1E;x6A;xEF;x26;xDF;xC0;x6D;x9A;xEE;x0B;xB9;xD2;x82;x4B;x51;x21;x10;x01;x5C;x18;xC4;x96;xB2;x60;x2A;xA2;x8A;x51;x01;x8B;xF6;x3C;x1B;x29;x9C;x93;x1B;x48;xCA;x8E;x02;x33;x76;x76;xAE;x16;)
    FIELD(x30;)LEN(x82;x04;xA3;)VALUE(all)
      FIELD(x02;)LEN(x01;)VALUE(x00;)
      FIELD(x02;)LEN(x82;x01;x01;)VALUE(x00;xC0;xDC;xD3;xE9;xB8;x05;x9F;x8D;xDD;xDB;xCD;x8B;x1A;x80;xB6;x02;xA3;x94;x93;x19;x6B;xFB;x37;x5A;x9A;xEF;xCE;x8C;x7D;xCF;xC2;x6C;x13;x89;xF9;xC3;xCD;x34;x2C;x2C;x1C;x85;xC8;x44;xB1;xA5;xF8;x97;x80;x12;xF2;x30;xB2;x6D;xA3;xD3;xC3;xBB;x1D;xFA;x6F;x0C;xE2;x87;x40;x14;x02;x14;xC5;xBF;xBD;x35;xB8;x11;xB5;xBE;x99;xA8;xD3;xF5;x6A;x66;xCA;xCA;xFD;xFB;x7A;x17;x8D;x2F;xEA;x3B;x76;xE5;x80;x97;xF5;x27;x9C;x71;x8C;x70;x70;x7C;x7E;x40;x2B;x7C;x2D;x83;x8C;xB6;x6D;x5D;x63;x78;x56;x2E;xBB;x32;x60;x73;xEE;xBA;xB8;x47;x3C;x3E;xE3;x19;x59;xB4;xD4;xB7;x11;xBC;xB7;x91;xC3;xC7;x1F;x9C;x8B;x46;xD5;x13;xF4;x5C;x22;xEF;x28;xCC;xF5;xDC;x43;x98;x61;xF8;x15;x18;x71;x3E;x5B;x55;x03;xA1;xED;x58;x20;x8F;x12;xFA;x2A;x59;xE3;x82;x9C;x38;xBE;xE5;x74;x29;x0B;x49;xCC;x19;xE8;xD9;x5E;xC1;x0A;xD5;x6B;x06;xCB;xE4;x3C;x0E;x00;xA2;x4F;x56;x5F;x5A;x98;x45;xED;xDA;x62;xC7;x76;x08;xA2;xBC;xDD;xB6;xBE;x93;x4C;xEA;x93;x12;xF8;xF8;xB7;x2D;xC8;x85;x46;x2D;x30;x09;xA3;x07;xAA;xD2;x02;x53;xFB;x10;xCC;x8E;x87;x27;xE5;x4C;xA2;x19;x32;xFE;x56;x83;xF9;x05;x10;xAF;)
      FIELD(x02;)LEN(x03;)VALUE(x01;x00;x01;)
      FIELD(x02;)LEN(x82;x01;x00;)VALUE(x12;x9C;xFD;x62;xC0;x1C;xE2;xD2;x39;x48;x3E;x65;x1F;x70;xAE;xA0;x40;x93;x55;x43;x0D;xD5;xD7;xF8;xC9;x9D;x3D;x4C;xF7;xD3;x76;x4B;x21;xF3;x9E;x04;x54;xA8;xA5;x5E;xB8;x7D;xFC;xDF;x0C;x5A;x1D;x4C;xD9;xD2;x7A;x47;x52;xE1;x1B;xFF;x93;x13;x5B;x08;x51;x71;x67;xE7;x6F;xBA;xBE;x9C;xAC;x9D;x1D;xB0;xB2;x8C;x1B;x03;x43;x27;x35;x15;xA9;x68;x34;x48;x35;x0C;xF1;x32;x96;xB0;xBA;x25;x75;x0B;x1F;x47;x0B;x25;x00;x87;x40;xEB;x95;x1F;x9E;xBB;xA8;xB6;xBA;x59;xC8;xD9;xCE;x62;x45;xF8;x56;x30;xFA;x66;x19;x42;xF7;x16;x54;x34;xA3;xCE;xAE;xC5;x19;x87;x3A;x74;x3E;x69;xA1;x68;x1B;xA7;xEB;xA0;xB6;x26;x0E;x50;xD7;xC0;x75;x5F;x68;xAC;x95;x20;x21;xD6;x80;x67;x58;x83;x9F;x2A;x45;xAA;xCE;x67;x0A;x74;xF3;xB0;x15;x4C;xF0;x23;x60;xDB;x72;x18;x71;x36;xD1;x23;x70;xDB;xD8;x1A;x85;x49;x8F;xE6;x8E;xA8;xCA;x52;xFC;x25;xCA;x81;x0F;xB2;xB2;x68;x9E;xC9;xEC;x88;xD9;x0E;xF9;x9F;x48;x15;x27;x08;x98;x95;x3B;xE6;x95;x97;xDB;xDE;x2C;x88;x70;x49;xB9;x6F;x0C;x47;xB1;xB2;x53;x37;x72;x27;xAC;x69;x3A;x2C;xDF;xB1;xF1;x8D;x6F;x31;x44;x52;x95;x81;xA3;x33;xED;xEB;x64;xE1;)
      FIELD(x02;)LEN(x81;x81;)VALUE(x00;xF9;x69;x9C;x4D;xB5;xB2;xBB;x9C;x3A;x4D;xB6;xE1;xDC;xB5;x7E;x74;xDA;x01;xE3;x7D;x18;xAC;xA7;xF2;x22;xF6;x34;x57;xE5;x2E;xED;x2B;x0F;xE0;x57;xFF;xAC;x4C;x14;xC9;x66;x17;x0C;x9D;xA0;x3C;xBF;x6E;x9C;xC8;xF9;x81;x41;xA1;x90;xA1;x6F;xC6;xF2;xE3;xB0;x54;x11;xFA;xA5;x33;x97;xF5;x2E;xE1;x32;x2F;x96;xA6;x4C;x7E;xC2;xE4;x81;xEE;x54;x7A;xBC;x00;x36;xE0;x57;xC1;x82;x3A;x58;xB4;x78;xD0;x3D;xED;xEF;x79;x19;x4B;xD0;x03;x55;xA5;x86;x90;xB0;x26;xA2;x1A;x0C;x96;xF8;x89;xAD;x44;x9E;x0E;x0E;xF9;x37;x45;x39;x42;xDA;x7A;x31;x51;)
      FIELD(x02;)LEN(x81;x81;)VALUE(x00;xC5;xF4;xDB;x9F;xB4;x78;x2D;xE6;x89;x94;xA1;xD7;x00;xBD;xEF;xEE;x55;x9D;x9E;x5B;x73;xFB;xEA;x0C;x40;x45;x94;x96;xCF;x32;x98;x49;x06;xF3;xB1;x9C;xD0;xE2;xC8;x7D;x51;x8E;xDA;x53;x17;x5B;x3A;x8C;x69;x28;xF8;x0E;x03;x68;x1C;x95;x75;xED;xF6;x28;xA1;x82;x70;xF7;xDF;xDB;xED;x1C;xE1;xE9;xF9;xA4;xDD;x62;x99;x0C;xAC;x44;x94;x71;xAC;xAA;x50;xB3;x80;x64;x5E;xDD;xDC;xCB;x84;x48;x2D;x0A;x17;x42;x1D;xA0;x4B;xF3;x40;x0F;x4D;x8A;x29;x1A;x1D;xB7;x21;x89;x1D;xB4;x8F;xC9;x62;x64;x71;x30;x1A;xC2;xA8;x0D;xC2;x00;x99;x2A;xA1;xFF;)
      FIELD(x02;)LEN(x81;x81;)VALUE(x00;x8A;xA1;x1C;x02;xC9;x8A;x95;xE3;x09;x39;x26;xFC;xB5;x9B;x9F;xB5;x3D;x73;xAD;x49;x5C;x0C;xA3;xB0;xDF;xA3;xEF;x86;x27;x5A;x04;xF3;x59;x78;xBE;x10;xDB;x68;xD2;x68;xFB;x38;xB6;x87;x6A;x88;x39;x73;x36;xEC;x32;x5A;x98;xEB;x3F;xA3;xAB;xA8;x6E;x5B;x06;x28;x44;x72;x07;x9E;xFC;xC8;x88;x0D;x1D;xC2;xFB;xBE;x65;x68;x53;xD5;x85;x2E;xBE;x80;x15;xBD;x1C;xC3;x67;xA3;xA4;x49;xE0;x02;x37;xE0;xAF;x7B;x70;x0E;xE4;x73;x92;x24;x38;x57;xAF;xCA;xFE;x4E;x0A;xED;xE5;xAF;x88;x67;xA6;x0A;x2C;xBB;xED;xB0;xB1;xFA;xE5;x0F;xF6;xB5;xD2;x71;)
      FIELD(x02;)LEN(x81;x80;)VALUE(x0F;x33;x05;xC9;xF5;x69;x64;xDC;xD6;xA3;x7A;xE9;xAF;x3C;xE6;x37;x3E;x8B;xA3;xA5;x11;xFA;xBD;xB5;xC3;x19;x94;x97;x1F;xC1;x9A;xBF;xC0;xB0;xE5;x6A;x4C;xFF;x9C;xB5;x42;x95;xDD;x5D;x93;xE5;x85;x51;x52;xA1;xBA;xAA;x18;xC8;xDD;xA8;xFC;x2D;x11;x41;x7D;x65;x2E;x97;x59;xB0;xE1;x3B;xDE;x7C;xC0;x96;x50;x09;x4A;x07;x17;x13;x0C;xF2;xCD;x77;x26;x4B;x22;x08;x92;xE0;x26;x1B;xDA;x44;x50;x70;xFD;xE8;x2D;xAE;x29;x26;xBC;x3C;x70;x8F;xB3;x28;x36;x2F;xE1;x7B;x4D;xB4;x97;x75;xB7;x00;x4F;x50;x8A;x3D;x77;xF9;xD6;x73;x85;x4F;x28;xB1;)
      FIELD(x02;)LEN(x81;x80;)VALUE(x39;x69;x6D;x5B;x26;x7A;xF4;x8F;x6A;x88;x45;xD9;x5A;xD8;x50;xC4;x53;x76;x10;x86;x6A;x5A;xA4;xF9;x16;xDD;xAC;xCE;x67;x7E;x93;x70;x12;xBA;xFB;x2B;xEA;xF9;x0F;x0B;xA1;xBB;xC0;x6F;x34;xAD;x9C;xAC;x8F;x23;xED;x46;x7B;xAA;x30;x3C;x1C;xCF;xA1;x3D;x64;xC8;xDB;x9A;x2B;xF9;x13;x13;xE9;xAA;x4D;x40;xC6;x0A;xCA;x44;x5E;x96;x8B;x4F;x59;x10;x1E;x6A;xEF;x26;xDF;xC0;x6D;x9A;xEE;x0B;xB9;xD2;x82;x4B;x51;x21;x10;x01;x5C;x18;xC4;x96;xB2;x60;x2A;xA2;x8A;x51;x01;x8B;xF6;x3C;x1B;x29;x9C;x93;x1B;x48;xCA;x8E;x02;x33;x76;x76;xAE;x16;)
 */


bool WolfSslRsaSign::sign(uint8_t *data, uint16_t dataLen) {
	LOG_DEBUG(LOG_FRP, "sign");
	uint8_t hash[64] = { 0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20 };
	uint32_t prefixLen = 19;
	uint32_t hashLen = WC_SHA256_DIGEST_SIZE;
	if(wc_Hash(WC_HASH_TYPE_SHA256, data, dataLen, hash + prefixLen, hashLen) != 0) {
		LOG_ERROR(LOG_FRP, "wc_Hash failed");
		return false;
	}
	hashLen += prefixLen;

	word32 signBufSize = sizeof(signBuf);
	int signLen = sizeof(signBuf);
	int signResult = wc_RsaSSL_Sign(hash, hashLen, signBuf, signBufSize, key, 0);
	if(signResult != signLen) {
		LOG_ERROR(LOG_FRP, "wc_RsaSSL_Sign failed " << signResult << "<>" << signLen);
		return false;
	}
	return true;
}

uint16_t WolfSslRsaSign::base64encode(uint8_t *buf, uint16_t bufSize) {
	LOG_DEBUG(LOG_FRP, "base64encode");
	word32 signLen = sizeof(signBuf);
	word32 sign64Len = bufSize;
	int ret = Base64_Encode_NoNl(signBuf, signLen, buf, &sign64Len);
	if(ret != 0) {
		LOG_ERROR(LOG_FRP, "Base64_Encode_NoNl failed");
		return 0;
	}
	return sign64Len;
}
