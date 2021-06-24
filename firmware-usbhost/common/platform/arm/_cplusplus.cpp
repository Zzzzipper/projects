#if 0
#ifndef _CPLUSPLUS_H
#define _CPLUSPLUS_H
 
#include <stdlib.h>
 
#ifdef AVR
/*
    This is applicable if using virtual inheritance.
*/
__extension__ typedef int __guard __attribute__((mode (__DI__)));
/*
 
extern "C" int __cxa_guard_acquire(__guard *);
extern "C" void __cxa_guard_release (__guard *);
extern "C" void __cxa_guard_abort (__guard *);
 
int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);};
void __cxa_guard_release (__guard *g) {*(char *)g = 1;};
void __cxa_guard_abort (__guard *) {}; 
 
 */
 
/*
    This is applicable if using pure virtual inheritance.
*/
//extern "C" void __cxa_pure_virtual(void); 
 
//void __cxa_pure_virtual(void) {}; 
 
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
#endif

#ifdef ARM

#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>

#include "memory/include/MallocTest.h"

#include "config.h"

extern uint32_t __get_MSP(void);

/*
 * The default pulls in 70K of garbage
 */
#undef errno
extern int errno;

/*
 ѕеременные среды - пустой список.
 */
char *__env[1] = { 0 };
char **environ = __env;

namespace __gnu_cxx {

void __verbose_terminate_handler() {
	for(;;);
}

}

/*
 * The default pulls in about 12K of garbage
 */
extern "C" void __cxa_pure_virtual() {
	for(;;);
}

/*
 * Implement C++ new/delete operators using the heap
 */
void *operator new(size_t size) {
#ifdef DEBUG_MEMORY
	return MallocMalloc(size);
#else
	return malloc(size);
#endif
}

void *operator new[](size_t size) {
#ifdef DEBUG_MEMORY
	return MallocMalloc(size);
#else
	return malloc(size);
#endif
}

void operator delete(void *p) {
#ifdef DEBUG_MEMORY
	MallocFree(p);
#else
	free(p);
#endif
}

void operator delete[](void *p) {
#ifdef DEBUG_MEMORY
	MallocFree(p);
#else
	free(p);
#endif
}

/*
 * sbrk function for getting space for malloc and friends
 */


//extern int  _end;
/*
extern "C" caddr_t _sbrk ( int incr ) {
    extern char _ebss;
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == 0)
    {
        heap_end = &_ebss;
    }
    prev_heap_end = heap_end;

    char * stack = (char*) __get_MSP();
    if (heap_end + incr > stack)
    {
//        _write(STDERR_FILENO, "Heap and stack collision\n", 25);
        errno = ENOMEM;
        return (caddr_t) -1;
        //abort ();
    }

    heap_end += incr;
    return (caddr_t) prev_heap_end;

  }
*/
extern "C" int _kill(int pid, int sig) {
	return -1;
}

extern "C" void _exit(int status) {
//    xprintf("_exit called with parameter %d\n", status);
	while(1) {;}
}

extern "C" int _getpid ( int incr ) {
	return 1;
}

#endif
#endif //_CPLUSPLUS_H
#else
#include "platform/include/platform.h"
#include "memory/include/MallocTest.h"

#include <malloc.h>
#include <stddef.h>

/*
 * Вызывается, когда обращаются к виртуальному методу, который не имеет реализации.
 */
extern "C" void __cxa_pure_virtual() {
	while(1);
}

/*
 * Implement C++ new/delete operators using the heap
 */
void *operator new(size_t size) {
	void *res;
	ATOMIC {
#ifdef DEBUG_MEMORY
		res = MallocMalloc(size);
#else
		res = malloc(size);
#endif
	}
	return res;
}

void *operator new[](size_t size) {
	void *res;
	ATOMIC {
#ifdef DEBUG_MEMORY
		res = MallocMalloc(size);
#else
		res = malloc(size);
#endif
	}
	return res;
}

void operator delete(void *p) {
	ATOMIC {
#ifdef DEBUG_MEMORY
		MallocFree(p);
#else
		free(p);
#endif
	}
}

void operator delete[](void *p) {
	ATOMIC {
#ifdef DEBUG_MEMORY
		MallocFree(p);
#else
		free(p);
#endif
	}
}

#endif
