#ifndef COMMON_PLATFORM_ARM_ATOMIC_H
#define COMMON_PLATFORM_ARM_ATOMIC_H

#include "cmsis_boot/stm32f4xx_conf.h"

// http://we.easyelectronics.ru/STM32/atomic-makrosy-dlya-arm.html
__attribute__((always_inline)) inline static int _iDisGetPrimask(void)
{
    int result;
    __ASM volatile ("MRS %0, primask" : "=r" (result) );
    __ASM volatile ("cpsid i" : : : "memory");
    return result;
}

__attribute__((always_inline)) inline static int _iSetPrimask(int priMask)
{
    __ASM volatile ("MSR primask, %0" : : "r" (priMask) : "memory");
    return 0;
}

/* TODO: Использование макроса ATOMIC_BLOCK_GLOBAL_RESTORESTATE
 * Макрос глобально запрещает все прерывания (PRIMASK). При выходе из макроса восстанавливается предыдущее состояние.
 * Использование:
 *  В config.h включить макрос #define ATOMIC 	ATOMIC_BLOCK_GLOBAL_RESTORESTATE
 * 	В коде использовать:
 * 	ATOMIC
 * 	{
 * 		// Атомарный код.
 * 		// Внутри блока нельзя использовать return, goto, break,
 * 		//  иначе статус регистров прерывания восстановлен не будет и приведет к блокировке !
 * 	}
 */
#define ATOMIC_BLOCK_GLOBAL_RESTORESTATE for(int _mask = _iDisGetPrimask(), _flag = 1; _flag; _flag = _iSetPrimask(_mask))

/* TODO: Использование макроса ATOMIC_BLOCK_CUSTOM_RESTORE_STATE
 *
 * Макрос отключает все прерывания через регистр NVIC, кроме тех, что добавлены в список исключений.
 * При выходе из макроса восстанавливается предыдущее состояние регистров прерывания.
 * Использование:
 * В начале программы вызвать:
 * 	ATOMIC_BLOCK_CUSTOM_CLEAR_ALL_IRQ();
 * Затем добавить нужные прерывания в список исключений:
 * 	ATOMIC_BLOCK_CUSTOM_REGISTER_EXCEPT_IRQ(USART1_IRQn);
 * 	ATOMIC_BLOCK_CUSTOM_REGISTER_EXCEPT_IRQ(UART4_IRQn);
 * 	...
 *
 * 	В config.h включить макрос #define ATOMIC 	ATOMIC_BLOCK_CUSTOM_RESTORESTATE
 * 	В коде использовать:
 * 	ATOMIC
 * 	{
 * 		// Почти атомарный код. Прерывания USART1_IRQn и UART4_IRQn остаются активными.
 * 		// Внутри блока нельзя использовать return, goto, break,
 * 		//  иначе статус регистров прерывания восстановлен не будет и приведет к блокировке !
 * 	}
 *
 * 	Если в список исключений не добавлять прерывания, то код в ATOMIC блоке будет полностью атомарным.
 *
 */
__attribute__((used)) static int ATOMIC_BLOCK_CUSTOM_EXCEPT_NVIC_IRQ_REGISTERS[3] = {0, 0, 0};

__attribute__((always_inline)) inline static void ATOMIC_BLOCK_CUSTOM_CLEAR_ALL_IRQ()
{
	ATOMIC_BLOCK_CUSTOM_EXCEPT_NVIC_IRQ_REGISTERS[0] = 0;
	ATOMIC_BLOCK_CUSTOM_EXCEPT_NVIC_IRQ_REGISTERS[1] = 0;
	ATOMIC_BLOCK_CUSTOM_EXCEPT_NVIC_IRQ_REGISTERS[2] = 0;
}

#define NVIC_INDEX(irq) 	((uint32_t)((int32_t)irq) >> 5)
#define NVIC_MASK(irq)		(uint32_t)(1 << ((uint32_t)((int32_t)irq) & (uint32_t)0x1F))

__attribute__((always_inline)) inline static void ATOMIC_BLOCK_CUSTOM_REGISTER_EXCEPT_IRQ(IRQn_Type IRQn)
{
	ATOMIC_BLOCK_CUSTOM_EXCEPT_NVIC_IRQ_REGISTERS[NVIC_INDEX(IRQn)] |= NVIC_MASK(IRQn);
}

__attribute__((always_inline)) inline static int _GetAndClearNVIC(int index)
{
    int r = 0;
    ATOMIC_BLOCK_GLOBAL_RESTORESTATE
	{
		r = NVIC->ISER[index];
		NVIC->ICER[index] = ~ATOMIC_BLOCK_CUSTOM_EXCEPT_NVIC_IRQ_REGISTERS[index];
	}
    return r;
}

__attribute__((always_inline)) inline static int _SetNVIC(int nv0, int nv1, int nv2)
{
	ATOMIC_BLOCK_GLOBAL_RESTORESTATE
	{
		NVIC->ISER[0] = nv0;
		NVIC->ISER[1] = nv1;
		NVIC->ISER[2] = nv2;
	}
    return 0;
}

#define ATOMIC_BLOCK_CUSTOM_RESTORESTATE \
	for(int _nv0 = _GetAndClearNVIC(0), _nv1 = _GetAndClearNVIC(1), _nv2 = _GetAndClearNVIC(2), _flag = 1; _flag; _flag = _SetNVIC(_nv0, _nv1, _nv2))

#include "config.h"

#ifdef ATOMIC_CUSTOM
#define ATOMIC ATOMIC_BLOCK_CUSTOM_RESTORESTATE
#else
#define ATOMIC ATOMIC_BLOCK_GLOBAL_RESTORESTATE
#endif

#endif
