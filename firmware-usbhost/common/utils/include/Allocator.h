#ifndef COMMON_UTILS_ALLOCATOR_H
#define COMMON_UTILS_ALLOCATOR_H

#include "platform/include/platform.h"

#include <stdint.h>
#include <stdlib.h>

#define MIN_BUF_SIZE 2

namespace Ephor {

template <typename T>
class Allocator {
public:
	virtual ~Allocator() {}
	virtual T* init(uint32_t *bufSize) = 0;
	virtual T* resize(uint32_t *bufSize, const uint32_t newSize) = 0;
};

template <typename T>
class StaticAllocator : public Allocator<T> {
public:
	StaticAllocator(void *buf, uint32_t bufSize) {
		p = (T*)buf;
		size = bufSize/sizeof(T);
	}

	T* init(uint32_t *bufSize) {
		*bufSize = size;
		return p;
	}

	T* resize(uint32_t *bufSize, const uint32_t) {
		*bufSize = size;
		return p;
	}

private:
	T* p;
	uint32_t size;
};

template <typename T>
class DynamicAllocator : public Allocator<T> {
public:
	DynamicAllocator(uint32_t minSize, uint32_t maxSize) : p(NULL) {
		this->minSize = (minSize < MIN_BUF_SIZE) ? MIN_BUF_SIZE : minSize;
		this->maxSize = (maxSize < MIN_BUF_SIZE) ? MIN_BUF_SIZE : maxSize;
		if(this->minSize > this->maxSize) { this->minSize = this->maxSize; }
	}

	~DynamicAllocator() {
		if(p != NULL) { delete p; }
	}

	T* init(uint32_t *bufSize) {
		p = new T[minSize];
		*bufSize = minSize;
		return p;
	}

	/*
	 * Устанавливает заданный размер буфера строки.
	 * Параметры:
	 *   bufSize - текущий размер буфера и буфер под новое значение
	 *   newSize - новый размер буфера
	 * Возвращаемое значение:
	 *   true - размер изменен или равен заданному
	 *   false - размер не был изменен
	 */
	T* resize(uint32_t *bufSize, const uint32_t newSize) {
		uint32_t size = *bufSize;
		if(newSize == size) return p;
		if(newSize < minSize && size == minSize) return p;
		if(newSize > maxSize && size == maxSize) return p;
		size = newSize;
		if(size < minSize) { size = minSize; }
		if(size > maxSize) { size = maxSize; }
		*bufSize = size;
		p = (T*)realloc((void *)p, size * sizeof(T));
		return p;
	}

private:
	uint32_t minSize;
	uint32_t maxSize;
	T *p;
};

}

#endif
