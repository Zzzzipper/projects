#ifndef AVERAGE__H
#define AVERAGE__H 

#include "platform/include/platform.h"


/*
	Класс шаблон по вычиследнию среднего значения массива.
	Вычисление производится непосредственно при добавлении нового значения.
	При заполнении заданного массива данными, выставляется флаг full. Дальнейшее добавление данных идет вытеснением самого старого значения.
	Необходимо следить за переполнением переменной sum при использовании большого массива и отрицательных чисел в unsigned представлении !
*/

template <typename T>
class FastAverage {
	private:
		T *t;
		int size;
		int index;
		long sum;
		uint16_t count;
		bool full;
	
	public:
		FastAverage(const int size)
		{
			t = new T[size];
			this->size = size;
			clear();
		}

		~FastAverage() {
			delete []t;
		}

		void clear() {
			ATOMIC {
				index = 0;
				sum = 0;
				count = 0;
				full = false;
				for (uint8_t i = 0; i < size; i++)
					t[i] = 0;
			}
		}
	
		void add(T value) {
			sum -= t[index];
			sum += value;
			count++;
			t[index++] = value;
			if (index == size) {
				index = 0;
				full = true;
			}
		}
	
		T getAverage() {
			T res;
			ATOMIC {
				if (!full && !index) res = 0;
				else res = sum/(full ? size : index);
			}
			return res;
		}
			
		uint16_t getCount() {
			// Сколько всего раз вызывался метод add.
			return count;
		}
		
		bool isFull() {
			return full;
		}

};


#endif
