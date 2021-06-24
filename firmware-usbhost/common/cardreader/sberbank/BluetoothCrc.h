#ifndef COMMON_SBERBANK_BLUETOOTHCRC_H
#define COMMON_SBERBANK_BLUETOOTHCRC_H

#include <stdint.h>

#define BLUETOOTH_CRC_SIZE 2

class BluetoothCrc {
public:
	void start();
	void add(uint8_t byte);
	void add(const uint8_t *data, uint16_t dataLen);
	uint16_t getCrc();
	uint8_t getHighByte();
	uint8_t getLowByte();

private:
	uint16_t crc;
};

#endif
