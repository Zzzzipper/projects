#ifndef COMMON_UTILS_NETWORKPROTOCOL_H
#define COMMON_UTILS_NETWORKPROTOCOL_H

#include "utils/include/Hex.h"

#include <stdint.h>

#pragma pack(push,1)

template <uint16_t _SIZE>
struct StrParam {
	uint8_t str[_SIZE + 1];

	StrParam() {
		str[0] = '\0';
	}
	StrParam(const char *str) {
		set(str);
	}
	uint16_t getSize() { return (_SIZE + 1); }
	void set(const char *str) {
		uint16_t size = _SIZE + 1;
		uint16_t i = 0;
		for(; i < _SIZE && str[i] != '\0'; i++) {
			this->str[i] = str[i];
		}
		for(; i < size; i++) {
			this->str[i] = '\0';
		}
	}
	void set(const char *str, uint16_t len) {
		uint16_t size = _SIZE + 1;
		uint16_t i = 0;
		for(; i < _SIZE && i < len; i++) {
			this->str[i] = str[i];
		}
		for(; i < size; i++) {
			this->str[i] = '\0';
		}
	}
	char *get() {
		return (char*)this->str;
	}
	const char *get() const {
		return (const char*)this->str;
	}
	bool equal(const char *value) {
		uint16_t i = 0;
		for(; this->str[i] != '\0' && value[i] != '\0'; i++) {
			if(this->str[i] != value[i]) {
				return false;
			}
		}
		if(this->str[i] != value[i]) {
			return false;
		}
		return true;
	}
	void clear() {
		uint16_t size = _SIZE + 1;
		for(uint16_t i = 0; i < size; i++) {
			this->str[i] = '\0';
		}
	}
};

template <typename _SIZETYPE, uint16_t _SIZE>
struct BinParam {
	_SIZETYPE len;
	uint8_t data[_SIZE];

	BinParam() {
		len = 0;
	}
	_SIZETYPE getSize() { return _SIZE; }
	void set(const uint8_t *data, _SIZETYPE dataLen) {
		len = (dataLen < _SIZE) ? dataLen : _SIZE;
		_SIZETYPE i = 0;
		for(; i < len; i++) {
			this->data[i] = data[i];
		}
		for(; i < _SIZE; i++) {
			this->data[i] = 0;
		}
	}
	uint8_t *getData() {
		return data;
	}
	_SIZETYPE getLen() const {
		return len;
	}
	void clear() {
		len = 0;
	}
};

struct BEUint2 {
	uint8_t value[2];

	BEUint2() {
		this->value[0] = 0;
		this->value[1] = 0;
	}

	void set(uint16_t value) {
		this->value[0] = value & 0xFF;
		this->value[1] = (value >> 8) & 0xFF;
	}

	uint16_t get() {
		uint32_t result = 0;
		result |= ((uint16_t)this->value[0]);
		result |= ((uint16_t)this->value[1]) << 8;
		return result;
	}
};

struct BEUint4 {
	uint8_t value[4];

	BEUint4() {
		this->value[0] = 0;
		this->value[1] = 0;
		this->value[2] = 0;
		this->value[3] = 0;
	}

	void set(uint32_t value) {
		this->value[0] = value & 0xFF;
		this->value[1] = (value >> 8) & 0xFF;
		this->value[2] = (value >> 16) & 0xFF;
		this->value[3] = (value >> 24) & 0xFF;
	}

	uint32_t get() {
		uint32_t result = 0;
		result |= ((uint32_t)this->value[0]);
		result |= ((uint32_t)this->value[1]) << 8;
		result |= ((uint32_t)this->value[2]) << 16;
		result |= ((uint32_t)this->value[3]) << 24;
		return result;
	}
};

struct BEUint5 {
	uint8_t value[5];

	BEUint5() {
		this->value[0] = 0;
		this->value[1] = 0;
		this->value[2] = 0;
		this->value[3] = 0;
		this->value[4] = 0;
	}

	void set(uint64_t value) {
		this->value[0] = value & 0xFF;
		this->value[1] = (value >> 8) & 0xFF;
		this->value[2] = (value >> 16) & 0xFF;
		this->value[3] = (value >> 24) & 0xFF;
		this->value[4] = (value >> 32) & 0xFF;;
	}

	uint64_t get() {
		uint64_t result = 0;
		result |= ((uint64_t)this->value[0]);
		result |= ((uint64_t)this->value[1]) << 8;
		result |= ((uint64_t)this->value[2]) << 16;
		result |= ((uint64_t)this->value[3]) << 24;
		result |= ((uint64_t)this->value[4]) << 32;
		return result;
	}
};

struct LEUint2 {
	uint8_t value[2];

	LEUint2() {
		this->value[0] = 0;
		this->value[1] = 0;
	}

	void set(uint16_t value) {
		this->value[0] = (value >> 8) & 0xFF;
		this->value[1] = value & 0xFF;
	}

	uint16_t get() {
		uint16_t result = 0;
		result |= (this->value[0]) << 8;
		result |= this->value[1];
		return result;
	}
};

struct LEUint3 {
	uint8_t value[3];

	LEUint3() {
		this->value[0] = 0;
		this->value[1] = 0;
		this->value[2] = 0;
	}

	void set(uint32_t value) {
		this->value[0] = (value >> 16) & 0xFF;
		this->value[1] = (value >> 8) & 0xFF;
		this->value[2] = value & 0xFF;
	}

	uint32_t get() {
		uint32_t result = 0;
		result |= ((uint32_t)this->value[0]) << 16;
		result |= ((uint32_t)this->value[1]) << 8;
		result |= ((uint32_t)this->value[2]);
		return result;
	}
};

struct LEUint4 {
	uint8_t value[4];

	LEUint4() {
		this->value[0] = 0;
		this->value[1] = 0;
		this->value[2] = 0;
		this->value[3] = 0;
	}

	void set(uint32_t value) {
		this->value[0] = (value >> 24) & 0xFF;
		this->value[1] = (value >> 16) & 0xFF;
		this->value[2] = (value >> 8) & 0xFF;
		this->value[3] = value & 0xFF;
	}

	uint32_t get() {
		uint32_t result = 0;
		result |= ((uint32_t)this->value[0]) << 24;
		result |= ((uint32_t)this->value[1]) << 16;
		result |= ((uint32_t)this->value[2]) << 8;
		result |= ((uint32_t)this->value[3]);
		return result;
	}
};

struct LEUint5 {
	uint8_t value[5];

	LEUint5() {
		this->value[0] = 0;
		this->value[1] = 0;
		this->value[2] = 0;
		this->value[3] = 0;
		this->value[4] = 0;
	}

	void set(uint64_t value) {
		this->value[0] = (value >> 32) & 0xFF;
		this->value[1] = (value >> 24) & 0xFF;
		this->value[2] = (value >> 16) & 0xFF;
		this->value[3] = (value >> 8) & 0xFF;
		this->value[4] = value & 0xFF;
	}

	uint64_t get() {
		uint64_t result = 0;
		result |= ((uint64_t)this->value[0]) << 32;
		result |= ((uint64_t)this->value[1]) << 24;
		result |= ((uint64_t)this->value[2]) << 16;
		result |= ((uint64_t)this->value[3]) << 8;
		result |= ((uint64_t)this->value[4]);
		return result;
	}
};

template <typename T>
T from2to(T number, uint8_t fromBase, uint8_t toBase) {
	T value1 = number;
	uint8_t rank = 0;
	T result = 0;
	while(value1 > 0) {
		T digit = value1%fromBase;
		value1 = value1/fromBase;
		for(uint8_t i = 0; i < rank; i++) {
			digit *= toBase;
		}
		result += digit;
		rank += 1;
	}
	return result;
}

struct Ubcd1 {
	uint8_t value[1];

	Ubcd1() {
		value[0] = 0;
	}

	void set(uint8_t value1) {
		value[0] = from2to<uint8_t>(value1, 10, 16);
	}

	uint8_t get() {
		return from2to<uint8_t>(value[0], 16, 10);
	}
};

struct LEUbcd2 {
	uint8_t value[2];

	LEUbcd2() {
		this->value[0] = 0;
		this->value[1] = 0;
	}

	void set(uint16_t value1) {
		uint16_t value2 = from2to<uint16_t>(value1, 10, 16);
		value[0] = (value2 >> 8) & 0xFF;
		value[1] = value2 & 0xFF;
	}

	uint16_t get() {
		uint16_t value1 = 0;
		value1 += ((uint16_t)value[0]) << 8;
		value1 += ((uint16_t)value[1]);
		return from2to<uint16_t>(value1, 16, 10);
	}
};

struct LEUbcd4 {
	uint8_t value[4];

	LEUbcd4() {
		this->value[0] = 0;
		this->value[1] = 0;
		this->value[2] = 0;
		this->value[3] = 0;
	}

	void set(uint32_t value1) {
		uint32_t value2 = from2to<uint32_t>(value1, 10, 16);
		value[0] = (value2 >> 24) & 0xFF;
		value[1] = (value2 >> 16) & 0xFF;
		value[2] = (value2 >> 8) & 0xFF;
		value[3] = value2 & 0xFF;
	}

	uint32_t get() {
		uint32_t value1 = 0;
		value1 += ((uint32_t)value[0]) << 24;
		value1 += ((uint32_t)value[1]) << 16;
		value1 += ((uint32_t)value[2]) << 8;
		value1 += ((uint32_t)value[3]);
		return from2to<uint32_t>(value1, 16, 10);
	}
};

struct LEUbcd5 {
	uint8_t value[5];

	LEUbcd5() {
		this->value[0] = 0;
		this->value[1] = 0;
		this->value[2] = 0;
		this->value[3] = 0;
		this->value[4] = 0;
	}

	void set(uint64_t value1) {
		uint64_t value2 = from2to<uint64_t>(value1, 10, 16);
		value[0] = (value2 >> 32) & 0xFF;
		value[1] = (value2 >> 24) & 0xFF;
		value[2] = (value2 >> 16) & 0xFF;
		value[3] = (value2 >> 8) & 0xFF;
		value[4] = value2 & 0xFF;
	}

	uint64_t get() {
		uint64_t value1 = 0;
		value1 += ((uint64_t)value[0]) << 32;
		value1 += ((uint64_t)value[1]) << 24;
		value1 += ((uint64_t)value[2]) << 16;
		value1 += ((uint64_t)value[3]) << 8;
		value1 += ((uint64_t)value[4]);
		return from2to<uint64_t>(value1, 16, 10);
	}
};

struct LEUbcd6 {
	uint8_t value[6];

	LEUbcd6() {
		this->value[0] = 0;
		this->value[1] = 0;
		this->value[2] = 0;
		this->value[3] = 0;
		this->value[4] = 0;
		this->value[5] = 0;
	}

	void set(uint64_t value1) {
		uint64_t value2 = from2to<uint64_t>(value1, 10, 16);
		value[0] = (value2 >> 40) & 0xFF;
		value[1] = (value2 >> 32) & 0xFF;
		value[2] = (value2 >> 24) & 0xFF;
		value[3] = (value2 >> 16) & 0xFF;
		value[4] = (value2 >> 8) & 0xFF;
		value[5] = value2 & 0xFF;
	}

	uint64_t get() {
		uint64_t value1 = 0;
		value1 += ((uint64_t)value[0]) << 40;
		value1 += ((uint64_t)value[1]) << 32;
		value1 += ((uint64_t)value[2]) << 24;
		value1 += ((uint64_t)value[3]) << 16;
		value1 += ((uint64_t)value[4]) << 8;
		value1 += ((uint64_t)value[5]);
		return from2to<uint64_t>(value1, 16, 10);
	}
};

struct LEUbcd7 {
	uint8_t value[7];

	LEUbcd7() {
		this->value[0] = 0;
		this->value[1] = 0;
		this->value[2] = 0;
		this->value[3] = 0;
		this->value[4] = 0;
		this->value[5] = 0;
		this->value[6] = 0;
	}

	void set(uint64_t value1) {
		uint64_t value2 = from2to<uint64_t>(value1, 10, 16);
		value[0] = (value2 >> 48) & 0xFF;
		value[1] = (value2 >> 40) & 0xFF;
		value[2] = (value2 >> 32) & 0xFF;
		value[3] = (value2 >> 24) & 0xFF;
		value[4] = (value2 >> 16) & 0xFF;
		value[5] = (value2 >> 8) & 0xFF;
		value[6] = value2 & 0xFF;
	}

	uint64_t get() {
		uint64_t value1 = 0;
		value1 += ((uint64_t)value[0]) << 48;
		value1 += ((uint64_t)value[1]) << 40;
		value1 += ((uint64_t)value[2]) << 32;
		value1 += ((uint64_t)value[3]) << 24;
		value1 += ((uint64_t)value[4]) << 16;
		value1 += ((uint64_t)value[5]) << 8;
		value1 += ((uint64_t)value[6]);
		return from2to<uint64_t>(value1, 16, 10);
	}
};

struct LEUnum1 {
	uint8_t value[1];

	LEUnum1() {
		this->value[0] = 0x30;
	}

	void set(uint16_t value1) {
		value[0] = value1 + 0x30;
	}

	uint16_t get() {
		uint16_t value1;
		value1 = (value[0] - 0x30);
		return value1;
	}
};

struct LEUnum2 {
	uint8_t value[2];

	LEUnum2() {
		this->value[0] = 0x30;
		this->value[1] = 0x30;
	}

	void set(uint16_t value1) {
		uint16_t value2 = value1;
		value[1] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[0] = (value2 % 10) + 0x30;
	}

	uint16_t get() {
		uint16_t value1;
		value1 = (value[0] - 0x30);
		value1 = value1 * 10 + (value[1] - 0x30);
		return value1;
	}
};

struct LEUnum3 {
	uint8_t value[3];

	LEUnum3() {
		this->value[0] = 0x30;
		this->value[1] = 0x30;
		this->value[2] = 0x30;
	}

	void set(uint16_t value1) {
		uint16_t value2 = value1;
		value[2] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[1] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[0] = (value2 % 10) + 0x30;
	}

	uint16_t get() {
		uint16_t value1;
		value1 = (value[0] - 0x30);
		value1 = value1 * 10 + (value[1] - 0x30);
		value1 = value1 * 10 + (value[2] - 0x30);
		return value1;
	}
};

struct LEUnum4 {
	uint8_t value[4];

	LEUnum4() {
		this->value[0] = 0x30;
		this->value[1] = 0x30;
		this->value[2] = 0x30;
		this->value[3] = 0x30;
	}

	void set(uint16_t value1) {
		uint16_t value2 = value1;
		value[3] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[2] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[1] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[0] = (value2 % 10) + 0x30;
	}

	uint16_t get() {
		uint16_t value1;
		value1 = (value[0] - 0x30);
		value1 = value1 * 10 + (value[1] - 0x30);
		value1 = value1 * 10 + (value[2] - 0x30);
		value1 = value1 * 10 + (value[3] - 0x30);
		return value1;
	}
};

struct LEUnum6 {
	uint8_t value[6];

	LEUnum6() {
		this->value[0] = 0x30;
		this->value[1] = 0x30;
		this->value[2] = 0x30;
		this->value[3] = 0x30;
		this->value[4] = 0x30;
		this->value[5] = 0x30;
	}

	void set(uint32_t value1) {
		uint32_t value2 = value1;
		value[5] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[4] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[3] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[2] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[1] = (value2 % 10) + 0x30; value2 = value2 / 10;
		value[0] = (value2 % 10) + 0x30;
	}

	uint32_t get() {
		uint32_t value1;
		value1 = (value[0] - 0x30);
		value1 = value1 * 10 + (value[1] - 0x30);
		value1 = value1 * 10 + (value[2] - 0x30);
		value1 = value1 * 10 + (value[3] - 0x30);
		value1 = value1 * 10 + (value[4] - 0x30);
		value1 = value1 * 10 + (value[5] - 0x30);
		return value1;
	}
};

struct LEUhex4 {
	uint8_t value[4];

	LEUhex4() {
		this->value[0] = 0;
		this->value[1] = 0;
		this->value[2] = 0;
		this->value[3] = 0;
	}

	void set(uint32_t value1) {
		value[0] = digitToHex((value1 >> 12) & 0xF);
		value[1] = digitToHex((value1 >> 8) & 0xF);
		value[2] = digitToHex((value1 >> 4) & 0xF);
		value[3] = digitToHex(value1 & 0xF);
	}

	uint32_t get() {
		uint32_t value1 = 0;
		value1 += hexToDigit(value[0]) << 12;
		value1 += hexToDigit(value[1]) << 8;
		value1 += hexToDigit(value[2]) << 4;
		value1 += hexToDigit(value[3]);
		return value1;
	}
};

#pragma pack(pop)

#endif
