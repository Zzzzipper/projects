#ifndef COMMON_DEX_CRC_H
#define COMMON_DEX_CRC_H

#include <stdint.h>

namespace Dex {
class Crc {
public:
	Crc();
    void start();
    void addUint8(const uint8_t value);
    void add(const char *string);
	void add(const void *data, const uint16_t len);
    uint8_t getHighByte();
    uint8_t getLowByte();
	
private:
    uint16_t crc;

	Crc( const Crc &c );
	Crc& operator=( const Crc &c );
};
}
#endif
