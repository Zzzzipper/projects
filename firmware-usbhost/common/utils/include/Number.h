#ifndef COMMON_UTILS_NUMBER_H_
#define COMMON_UTILS_NUMBER_H_

#include <stdint.h>

namespace Sambery {

#define STRING_FROM_UINT8_MAX_SIZE 4
#define STRING_FROM_UINT16_MAX_SIZE 6
#define STRING_FROM_UINT32_MAX_SIZE 10

/**
 * Преобразует число в строку.
 * Возвращаемое значение:
 *   размер в символах получившейся строки
 */
template <typename T>
uint16_t numberToString(const T num, char *buf, uint16_t bufSize) {
	char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	T n = (num > 0) ? num : -num;
	T d;
	int i = bufSize - 1;
	buf[i] = '\0';
	if(i < 0) {
		return 0;
	}
	while(i > 0) {
		i--;
		d = n % 10;
		buf[i] = digits[d];
		n = n / 10;
		if(n == 0) {
			break;
		}
	}
	if(num < 0 && i > 0) {
		i--;
		buf[i] = '-';
	}
	int k;
	for(k = 0; i < bufSize; k++, i++ ) {
		buf[k] = buf[i];
	}
	return (k - 1);
}

/**
 * Читает число из строки до первого символа не являющегося цифрой.
 * Если в начале строки не будет ни одной цифры, то вернет ноль в качестве значения числа.
 * Параметры:
 *   указатель на первый символ, распознаваемой строки
 *   указатель на указатель, который после выполнения будет установлен на последний символ-нецифру
 *     Если равен NULL, то не используется.
 * Возвращаемое значение:
 *   значение числа
 */
template <typename T>
T stringToNumber(const char *str, const char **last = 0) {
	char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	T result = 0;
	const char *s = str;
	for(; *s != '\0'; s++) {
		uint16_t i;
		for(i = 0; i < sizeof(digits); i++) {
			if(*s == digits[i]) {
				result = result*10 + i;
				break;
			}
		}
		if(i >= sizeof(digits)) {
			break;
		}
	}
	if(last != 0) {
		*last = s;
	}
	return result;
}
#if 0
template <typename T>
T stringToNumber(const char *str, uint16_t len, const char **last = 0) {
	char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	T result = 0;
	const char *s = str;
	const char *end = str + len;
	for(; s < end; s++) {
		uint16_t i;
		for(i = 0; i < sizeof(digits); i++) {
			if(*s == digits[i]) {
				result = result*10 + i;
				break;
			}
		}
		if(i >= sizeof(digits)) {
			break;
		}
	}
	if(last != 0) {
		*last = s;
	}
	return result;
}
#else
template <typename T>
uint16_t stringToNumber(const char *str, uint16_t len, T *value) {
	char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	T result = 0;
	const char *s = str;
	const char *end = str + len;
	for(; s < end; s++) {
		uint16_t i;
		for(i = 0; i < sizeof(digits); i++) {
			if(*s == digits[i]) {
				result = result*10 + i;
				break;
			}
		}
		if(i >= sizeof(digits)) {
			break;
		}
	}
	*value = result;
	return (s - str);
}
#endif
template <typename T>
uint16_t stringToNumber(const char *str, uint16_t len, uint16_t max, T *value) {
	char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	T result = 0;
	const char *s = str;
	const char *end = (len < max) ?  (str + len) : (str + max);
	for(; s < end; s++) {
		uint16_t i;
		for(i = 0; i < sizeof(digits); i++) {
			if(*s == digits[i]) {
				result = result*10 + i;
				break;
			}
		}
		if(i >= sizeof(digits)) {
			break;
		}
	}
	*value = result;
	return (s - str);
}

}

#endif /* UTILS_H_ */
