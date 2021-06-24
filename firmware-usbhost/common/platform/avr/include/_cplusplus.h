#ifndef _CPLUSPLUS_H
#define _CPLUSPLUS_H
 
#include <stdlib.h>

/*
    This is applicable if using virtual inheritance.
*/
__extension__ typedef int __guard __attribute__((mode (__DI__)));
 
 /*
	http://www.avrfreaks.net/forum/avr-c-micro-how
 */
 extern "C" int __cxa_guard_acquire(__guard *);
 extern "C" void __cxa_guard_release (__guard *);
 extern "C" void __cxa_guard_abort (__guard *);
 extern "C" void __cxa_pure_virtual(void);
 
/*
    Operators required for C++
*/

inline void* operator new(size_t size);
inline void operator delete(void* size);
inline void *operator new[](size_t size);
inline void operator delete[](void* size); 

void * operator new(size_t size)
{
    return malloc(size);
}
void * operator new[](size_t size)
{
	return malloc(size);
}
 
void operator delete(void* ptr)
{
    free(ptr);
}

void operator delete[](void* ptr)
{
	free(ptr);
}

#endif //_CPLUSPLUS_H