#ifndef FIRMWARE_H
#define FIRMWARE_H

#include "common/utils/include/sha1.h"

#include <stdint.h>

class Firmware {
public:
	Firmware(uint8_t *startAddress);
	uint32_t getHeaderSequence();
	uint32_t getApplicationSize();
	uint32_t getHardwareVersion();
	uint32_t getSoftwareVersion();
	uint32_t getCrcSequence();
	uint8_t *getCrc();
	uint32_t getCrcSize();
	void updateCrc();
	bool checkSequence();
	bool checkCrc();
	void print();

private:
	uint8_t *startAddress;
	sha1nfo crc;

	uint8_t *getHeaderAddress();
	uint8_t *getCrcAddress();
	void calcCrc();
};

#endif
