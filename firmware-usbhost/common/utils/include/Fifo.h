#ifndef __FIFO_H
#define __FIFO_H

#include "Allocator.h"

#include "platform/include/platform.h"

#include <stdint.h>

/*
  Класс - шаблон реализует последовательную очередь элементов. Первый положил, первый взял.
*/
template <typename T>
class Fifo {
public:
	Fifo(uint32_t bufSize): readIndex(0), writeIndex(0), cnt(0) {
		allocator = new Ephor::DynamicAllocator<T>(bufSize, bufSize);
		queue = allocator->init(&size);
	}

	Fifo(uint32_t bufSize, uint8_t *buf): readIndex(0), writeIndex(0), cnt(0) {
		allocator = new Ephor::StaticAllocator<T>(buf, bufSize);
		queue = allocator->init(&size);
	}

	~Fifo() {
		clear();
		delete allocator;
	}

	void clear() {
		ATOMIC {
			readIndex = 0;
			writeIndex = 0;
			cnt = 0;
		}
	}

	bool isEmpty() {
		return cnt == 0;
	}

	bool isFull() {
		return cnt >= size;
	}

	bool push(T t) {
		if(isFull() == true) { return false; }
		ATOMIC {
			queue[writeIndex] = t;
			writeIndex = nextWriteIndex();
			cnt++;
		}
		return true;
	}

	T pop() {
		T result = 0;
		ATOMIC {
			if(isEmpty() == false) {
				result = queue[readIndex];
				readIndex = nextReadIndex();
				cnt--;
			}
		}
		return result;
	}

	T get(uint32_t index) {
		return queue[index];
	}

	uint32_t getSize() {
		return cnt;
	}

private:
	Ephor::Allocator<T> *allocator;
	uint32_t size;
	T *queue;
	volatile uint32_t readIndex;
	volatile uint32_t writeIndex;
	volatile uint32_t cnt;

	uint32_t nextWriteIndex() {
		writeIndex = writeIndex + 1;
		if(writeIndex >= size) { return 0; }
		return writeIndex;
	}

	uint32_t nextReadIndex() {
		readIndex = readIndex + 1;
		if(readIndex >= size) { return 0; }
		return readIndex;
	}
};

#endif
