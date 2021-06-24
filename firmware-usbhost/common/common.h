#ifndef COMMON_COMMON_H__
#define COMMON_COMMON_H__

#include <stddef.h>

//todo: перенести макросы из этого файла в соответствующие инклюдники библиотеки
typedef unsigned char       BYTE;
typedef unsigned short      WORD; // должно быть 2 байта
typedef unsigned long       DWORD, DWORD_PTR; // должно быть 4 байта
typedef long                LONG;
typedef BYTE                RESULT;

#define MAKEWORD(a, b)      ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD_PTR)(l) >> 16))
#define LOBYTE(w)           ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((DWORD_PTR)(w) >> 8))

template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

#define MASK(x)	(1 << (x))
#define ntohs(n) (n >> 8 | n << 8)
#define htons(n) (n >> 8 | n << 8)

#endif
