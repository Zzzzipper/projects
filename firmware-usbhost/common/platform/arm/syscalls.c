/**************************************************************************//*****
 * @file     stdio.c
 * @brief    Implementation of newlib syscall
 ********************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include "cmsis_boot/stm32f4xx_conf.h"

#include "config.h"


//#include "SEGGER_RTT.h"

#undef errno
extern int errno;
extern int _end;

/*
 * Вызывается при расширении размера кучи.
 * Параметры:
 *   incr - размер дополнительно выделяемой области
 * Результат:
 *   адрес, до которого разрешено расширить кучу;
 *   -1 - нет свободной памяти
 */
/*This function is used for handle heap option*/
__attribute__ ((used))
caddr_t _sbrk ( int incr )
{
	extern int _end;
	extern int _eram;
	static caddr_t heap_end = NULL;
	caddr_t prev_heap_end;
	if(heap_end == 0) {
		heap_end = (caddr_t) &_end;
	}

	prev_heap_end = heap_end;
	caddr_t ram_end = (caddr_t) &_eram;
	if((heap_end + incr + STACK_MAX_SIZE) > ram_end) {
		errno = ENOMEM;
		return (caddr_t) -1;
	}

	heap_end += incr;
	return (caddr_t)prev_heap_end;
}

__attribute__ ((used))
int link(char *old, char *new)
{
    return -1;
}

__attribute__ ((used))
int _close(int file)
{
    return -1;
}

__attribute__ ((used))
int _fstat(int file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

__attribute__ ((used))
int _isatty(int file)
{
    return 1;
}

__attribute__ ((used))
int _lseek(int file, int ptr, int dir)
{
    return 0;
}

/*Low layer read(input) function*/
__attribute__ ((used))
int _read(int file, char *ptr, int len)
{

#if 0
     //user code example
     int i;
     (void)file;

     for(i = 0; i < len; i++)
     {
        // UART_GetChar is user's basic input function
        *ptr++ = UART_GetChar();
     }

#endif

    return len;
}

__attribute__((weak)) void _logPutChar(char c)
{
	// TODO: Переопределить для вывода через printf();
	// См: setvbuf(stdout, NULL, _IOFBF, 3); printf("Hello World!\n"); fflush(stdout);
}

/*Low layer write(output) function*/
__attribute__ ((used))
int _write(int file, char *ptr, int len)
{

#if 1
     for(int i = 0; i < len; i++)
        _logPutChar(*ptr++);
#endif

    (void) file;  /* Not used, avoid warning */
//    SEGGER_RTT_Write(0, ptr, len);
    return len;
}

__attribute__ ((used))
void abort(void)
{
    /* Abort called */
    while(1);
}

/* --------------------------------- End Of File ------------------------------ */
