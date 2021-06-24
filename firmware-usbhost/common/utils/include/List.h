#ifndef __LIST_H
#define __LIST_H

#include "platform/include/platform.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Класс-шаблон, хранит список объектов, созданных через new
 * В деструкторе автоматически очищает список и освобождает память
 * Пример использования смотри в Tests::testList()
 *
 */

#define LIST_DEFFAULT_SIZE 8
#define LIST_MIN_SIZE 2
#define LIST_DEFFAULT_STEP 4
#define LIST_MIN_STEP 2

template <class T>
class List {
public:
	List(): startSize(LIST_DEFFAULT_SIZE), step(LIST_DEFFAULT_STEP) {
		init();
	}

	List(int startSize): startSize(startSize), step(LIST_DEFFAULT_STEP) {
		init();
	}

	List(int startSize, int step) :startSize(startSize), step(step) {
		init();
	}

	virtual ~List() {
		clearAndFree();
		free(ptr);
	}
	
	void clearAndFree() {
		ATOMIC {
			for(int i = 0; i < index; i++) {
				delete ptr[i];
			}
			clear();
		}
	}
	
	void clear() {
		ATOMIC {
			index = 0;
			if((startSize + step) < size) {
				size = startSize + step;
				ptr = (T**)realloc(ptr, sizeof(T *) * size);
			}
		}
	}
	
	bool isEmpty() const {
		return index == 0;
	}
	
	uint16_t getSize() const {
		return index;
	}
	
	List *add(T *p) {
		List *res = this;
		ATOMIC {
			if(index >= size) {
				size += step;
				ptr = (T**)realloc(ptr, sizeof(T *) * size);
			}

			ptr[index++] = p;
		}
		return res;
	}
	
	List *remove(T *p) {
		ATOMIC {
			int i;
			for(i = 0; i < index; i++) {
				if(ptr[i] == p) {
					break;
				}
			}

			if(i < index) {
				for(; i < index; i++) {
					ptr[i] = ptr[i+1];
				}
				index--;
				ptr[index] = NULL;
			}
		}
		return this;
	}
	
	T *get(int index) {
		if(index >= this->index) {
			return NULL;
		}
		return ptr[index];
	}

	const T *get(int index) const {
		if(index >= this->index) {
			return NULL;
		}
		return ptr[index];
	}

	class Comparator {
	public:
		/**
		 * @brief Сравнивает два соседних узла списка.
		 * @param f - первый узел списка
		 * @param s - второй узел списка
		 * @return
		 * меньше нуля - f < s
		 * равно  нулю - f = s
		 * больше нуля - f > s
		 */
		virtual int compare(T *f, T *s) = 0;
	};

	/*
	 * Сортирует список по возростанию.
	 * Если необходимо перевернуть сортировку, то нужно изменить компаратор.
	 */
	void sort(Comparator *c) {
		if(index <= 1) {
			return;
		}
		for(int k = (index - 1); k > 0; k--) {
			for(int i = 0; i < k; i++) {
				T *f = ptr[i];
				T *s = ptr[i + 1];
				if(c->compare(f, s) > 0) {
					ptr[i] = s;
					ptr[i + 1] = f;
				}
			}
		}
	}

private:
	T **ptr;

	// Размер списка при инициализации, по умолчанию LIST_DEFFAULT_SIZE
	const int startSize;
	// Шаг добавления элементов списка при заполнении, по умолчанию LIST_DEFFAULT_STEP
	int step;
	// Текущий размер списка
	int size;
	// Кол-во элементов в списке
	int index;

	void init() {
		index = 0;
		size = startSize;
		if(size < LIST_MIN_SIZE) size = LIST_MIN_SIZE;
		if(step < LIST_MIN_STEP) step = LIST_MIN_STEP;
		size += step;

		ptr = (T**)malloc(sizeof(T *) * size);
	}

};

#endif
