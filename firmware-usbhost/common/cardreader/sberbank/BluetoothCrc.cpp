#include "BluetoothCrc.h"

//Протестировано
#define CRC16_DNP       0x3D65          // DNP, IEC 870, M-BUS, wM-BUS, ...
#define CRC16_CCITT     0x1021          // X.25, V.41, HDLC FCS, Bluetooth, ...

//Другие полиномы, не протестированы
#define CRC16_IBM       0x8005          // ModBus, USB, Bisync, CRC-16, CRC-16-ANSI, ...
#define CRC16_T10_DIF   0x8BB7          // SCSI DIF
#define CRC16_DECT      0x0589          // Радиотелефоны
#define CRC16_ARINC     0xA02B          // приложения ACARS

#define POLYNOM         CRC16_CCITT     // Подставить полином из списка выше

void BluetoothCrc::start() {
	crc = 0xFFFF;
}

void BluetoothCrc::add(uint8_t byte) {
	for(uint16_t i = 0; i < 8; i++) {
		if(((crc & 0x8000) >> 8) ^ (byte & 0x80)){
			crc = (crc << 1) ^ POLYNOM;
		} else {
			crc = (crc << 1);
		}
		byte <<= 1;
	}
}

void BluetoothCrc::add(const uint8_t *data, uint16_t dataLen) {
	for(uint16_t i = 0; i < dataLen; i++) {
		add(data[i]);
	}
}

uint16_t BluetoothCrc::getCrc() {
	return crc;
}

uint8_t BluetoothCrc::getHighByte() {
	return (uint8_t)(crc >> 8);
}

uint8_t BluetoothCrc::getLowByte() {
	return (uint8_t)crc;
}
