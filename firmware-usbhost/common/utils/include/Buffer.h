#ifndef COMMON_UTILS_BUFFER_H_
#define COMMON_UTILS_BUFFER_H_

#include <stdint.h>

class Buffer {
public:
	Buffer(uint32_t size);
	~Buffer();
	void addUint8(uint8_t data);
	void add(const void *data, const uint32_t len);
	void setLen(uint32_t len);
	void remove(uint32_t pos, uint32_t num);
	void clear();
	const uint8_t &operator[](uint32_t index) const;
	uint8_t &operator[](uint32_t index);
	uint8_t *getData();
	const uint8_t *getData() const;
	uint32_t getLen() const;
	uint32_t getSize() const;

private:
	uint8_t *buf;
	uint32_t size;
	uint32_t len;
};

#endif /* BUFFER_H_ */
