#ifndef DEFINES_H
#define DEFINES_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx.h"

#include "config.h"

//--------------------------------------------------------------------------------------------

#define HW_MIN_VERSION HW_2_0_0
#define HW_MAX_VERSION HW_3_5_x

#ifndef HW_VERSION
#error "HW_VERSION must be defined"
#else
#if (HW_VERSION < HW_MIN_VERSION) || (HW_VERSION > HW_MAX_VERSION)
#error "HW_VERSION is invalid"
#endif

#endif

//--------------------------------------------------------------------------------------------
// Настройка соответствий УАРТОВ

#if (HW_VERSION == HW_2_0_0)
 #define DEX_UART				UART_4
 #define MDB_UART				UART_2
 #define MDB_INV_UART			UART_5 // Инверсное подключение к линии MDB
 #define MDB_MASTER_UART		MDB_INV_UART
 #define DEBUG_UART				UART_1
 #define SIM900_UART			UART_3
#elif (HW_VERSION >= HW_3_0_0)
 #define DEX_UART				UART_4
 #define MDB_UART				UART_6
 #define MDB_MASTER_UART		UART_3
 #define DEBUG_UART				UART_1
 #define SIM900_UART			UART_2

#else
 #error "HW_VERSION must be defined"
#endif



#define FALSE	0
#define TRUE 1
typedef int BOOL;

//--------------------------------------------------------------------------------------------
// Версия прошивки формируется из 3-х частей: VersionHW.VersionH.VersionL
// Верcию, структуру заголовка, можно посмотреть в файлах link.ld

// Константы областей памяти частей программ
#define FLASH_SIZE					(0x100000) /* 1 MByte */
#define FLASH_SIZE_SIZEOFF			0x188	// Смещение области с размером прошивки. Задается в link.ld

/*
0. 0x08000000-0x08003FFF (16 кБ)
1. 0x08004000-0x08007FFF (16 кБ)
2. 0x08008000-0x0800BFFF (16 кБ)
3. 0x0800C000-0x0800FFFF (16 кБ)
4. 0x08010000-0x0801FFFF (64 кБ)
5. 0x08020000-0x0803FFFF (128 кБ)
6. 0x08040000-0x0805FFFF (128 кБ)
7. 0x08060000-0x0807FFFF (128 кБ)
8. 0x08080000-0x0809FFFF (128 кБ)
9. 0x080A0000-0x080BFFFF (128 кБ)
10. 0x080C0000-0x080DFFFF (128 кБ)
11. 0x080E0000-0x080FFFFF (128 кБ)
*/
#define BOOTLOADER_ADDRESS		0x08000000
#define	BOOTLOADER_MAX_SIZE		(256*1024) // Сектора 0-5
#define APPLICATION_OFFSET		BOOTLOADER_MAX_SIZE
#define APPLICATION_ADDRESS		(BOOTLOADER_ADDRESS + APPLICATION_OFFSET)
#define APPLICATION_PAGE_SIZE	0x20000 // 128 Kb - Размер страницы
#define APPLICATION_MAX_SIZE	(APPLICATION_PAGE_SIZE*5) // 128*5 Kb - Максимальный размер приложения - 5 страниц

#define CONFIG_SECTOR         FLASH_Sector_11 // последний сектор
#define CONFIG_SECTOR_ADDRESS 0x080E0000      // последний сектор
#define CONFIG_SECTOR_SIZE    0x20000         // размер 128к

#define APB_SPEED				84		// Скорость шины таймера, в Mhz
#define I2C_DEFAULT_SPEED		400000
#define I2C_SCREEN_SPEED		100000
#define I2C_SCREEN_RETRY_PAUSE	10


//#define SCREEN_DISABLE_CHECK_CRC

//--------------------------------------------------------------------------------------------
#define SIM900_SYNC_TRANSACTIONS_SIZE		8
#define SIM900_TRANSACTIONS_SIZE			16

#define ADC_PROCESS_AVERAGE_COUNT			8	// Кол-во замеров ADC для получения среднего

#if (HW_VERSION == HW_2_0_0)
 #define SIM900_KEY_AND_LED_PORT			GPIOE
 #define SIM900_PWR_KEY						GPIO_Pin_2
 #define SIM900_SOFT_RESET_KEY				GPIO_Pin_3
 #define SIM900_STATUS						GPIO_Pin_4
#elif (HW_VERSION >= HW_3_0_0 && HW_VERSION < HW_3_3_0)
#define SIM900_KEY_AND_LED_PORT				GPIOE
#define SIM900_PWR_KEY						GPIO_Pin_9
#define SIM900_SOFT_RESET_KEY				GPIO_Pin_3
#define SIM900_STATUS						GPIO_Pin_5

// TODO: Постараться использовать ETH_RESET и ETH_IRQ в модуле Enc28j60
#define ETH_RESET_PORT						GPIOB
#define ETH_RESET_PIN						GPIO_Pin_6
#define ETH_IRQ_PORT						GPIOE
#define ETH_IRQ_PIN							GPIO_Pin_4

// TODO: Выводы для отладки. Можно использовать совместно с макросами PROBE из platform.h
#define TEST_PORT	 						GPIOB
#define TEST_PIN 							GPIO_Pin_12

#define RELE1_PORT							GPIOD
#define RELE1_PIN							GPIO_Pin_11
#define RELE1_ON							GPIO_ResetBits(RELE1_PORT, RELE1_PIN);
#define RELE1_OFF							GPIO_SetBits(RELE1_PORT, RELE1_PIN);

#define BATTERY_PORT						GPIOE
#define BATTERY_PIN							GPIO_Pin_15
#define BATTERY_ENABLE						GPIO_ResetBits(BATTERY_PORT, BATTERY_PIN);
#define BATTERY_DISABLE						GPIO_SetBits(BATTERY_PORT, BATTERY_PIN);

#define EE_ADDRESS							EE_24LC256_ADDRESS
#define EE_MAX_SIZE							EE_24LC256_MAX_SIZE
#define EE_PAGE_SIZE						EE_24LC256_PAGE_SIZE

#define SCREEN_I2C							I2C_1
#define SCREEN_RFID_TIMEOUT					500	// Как часто происходит опрос данных RFID метки

#define USB_CDC_DEFAULT_RX_TX_BUFFER_SIZE	256 // Размер буферов по умолчанию

#elif (HW_VERSION >= HW_3_3_0)

#define SIM900_KEY_AND_LED_PORT				GPIOE
#define SIM900_PWR_KEY						GPIO_Pin_9
#define SIM900_STATUS						GPIO_Pin_5

	#if (HW_VERSION < HW_3_5_x)
	// TODO: Постараться использовать ETH_RESET и ETH_IRQ в модуле Enc28j60
	#define ETH_RESET_PORT						GPIOB
	#define ETH_RESET_PIN						GPIO_Pin_6
	#define ETH_IRQ_PORT						GPIOE
	#define ETH_IRQ_PIN							GPIO_Pin_4
	#endif

// TODO: Выводы для отладки. Можно использовать совместно с макросами PROBE из platform.h
#define TEST_PORT	 						GPIOB
#define TEST_PIN 							GPIO_Pin_12

#define RELE1_PORT							GPIOD
#define RELE1_PIN							GPIO_Pin_11
#define RELE1_ON							GPIO_ResetBits(RELE1_PORT, RELE1_PIN);
#define RELE1_OFF							GPIO_SetBits(RELE1_PORT, RELE1_PIN);

#define BATTERY_PORT						GPIOE
#define BATTERY_PIN							GPIO_Pin_15
#define BATTERY_ENABLE						GPIO_ResetBits(BATTERY_PORT, BATTERY_PIN);
#define BATTERY_DISABLE						GPIO_SetBits(BATTERY_PORT, BATTERY_PIN);

#define EE_ADDRESS							EE_M24M01_ADDRESS
#define EE_MAX_SIZE							EE_M24M01_MAX_SIZE
#define EE_PAGE_SIZE						EE_M24M01_PAGE_SIZE

#define UART_FORWARDING_PORT				GPIOD
#define UART_FORWARDING_PIN					GPIO_Pin_14
#define UART_FORWARDING_ENABLE				GPIO_ResetBits(UART_FORWARDING_PORT, UART_FORWARDING_PIN);
#define UART_FORWARDING_DISABLE				GPIO_SetBits(UART_FORWARDING_PORT, UART_FORWARDING_PIN);

#define USB_PROCESSING_PERIOD				25	// Период обработки главного цикла USB, млс
#define USB_CDC_DEFAULT_RX_TX_BUFFER_SIZE	256 // Размер буферов по умолчанию

#define SCREEN_I2C							I2C_1
#define SCREEN_RFID_TIMEOUT					500	// Как часто происходит опрос данных RFID метки

#else
#error "HW_VERSION must be defined"
#endif


//--------------------------------------------------------------------------------------------

// Настраиваем библиотеку RFID/NFC на работу без прерывания.
#define RFID_WITHOUT_IRQ

//--------------------------------------------------------------------------------------------
//------------ Н А С Т Р О Й К А  П Р И О Р И Т Е Т О В  П Р Е Р Ы В А Н И Й -----------------
//--------------------------------------------------------------------------------------------
/*
 Приоритет прерывания может состоять из 2 значений: самого приоритета прерывания и приоритета в подгруппе.
 Конфигурация группы/подгруппы определяется макросом IRQ_NVIC_PRIORITY_GROUP и может быть: NVIC_PriorityGroup_0 - NVIC_PriorityGroup_4
 Для конфигурации NVIC_PriorityGroup_3:
  IRQ_PRIORITY: 	0-7
  IRQ_SUB_PRIORITY:	0-1

1. Внутри группы прерывания не могут перебивать друг друга.
2. Прерывания из группы с меньшим номером могут перебивать прерывания из группы с большим номером.
3. Приоритет в подгруппе определяет порядок вызова обработчиков при одновременном возникновении прерываний из одной группы.

Статья: http://easyelectronics.ru/arm-uchebnyj-kurs-preryvaniya-i-nvic-prioritetnyj-kontroller-preryvanij.html
*/

#define IRQ_NVIC_PRIORITY_GROUP				NVIC_PriorityGroup_3

// EXTI, Port forwarding
#define IRQ_PRIORITY_EXTI					0
#define IRQ_SUB_PRIORITY_EXTI				0

// USB
#define IRQ_PRIORITY_USB					0
#define IRQ_SUB_PRIORITY_USB				1

// USB Processing Timer
#define IRQ_PRIORITY_TIM7					2
#define IRQ_SUB_PRIORITY_TIM7				0

// Ticker
#define IRQ_PRIORITY_TIM4					1
#define IRQ_SUB_PRIORITY_TIM4				0

// Debug
#define IRQ_PRIORITY_USART1					3
#define IRQ_SUB_PRIORITY_USART1				1

// SIM900
#define IRQ_PRIORITY_USART2					2
#define IRQ_SUB_PRIORITY_USART2				0

// MDB Master
#define IRQ_PRIORITY_USART3					2
#define IRQ_SUB_PRIORITY_USART3				0

// DEX
#define IRQ_PRIORITY_UART4					2
#define IRQ_SUB_PRIORITY_UART4				0

// -
#define IRQ_PRIORITY_UART5					2
#define IRQ_SUB_PRIORITY_UART5				0

// MDB Slave
#define IRQ_PRIORITY_USART6					2
#define IRQ_SUB_PRIORITY_USART6				0

// Ext 4 pin I2C
#define IRQ_PRIORITY_I2C1_DMA_RX			5
#define IRQ_SUB_PRIORITY_I2C1_DMA_RX		0

#define IRQ_PRIORITY_I2C1_DMA_TX			5
#define IRQ_SUB_PRIORITY_I2C1_DMA_TX		0

// EEPROM
#define IRQ_PRIORITY_I2C3_DMA_RX			5
#define IRQ_SUB_PRIORITY_I2C3_DMA_RX		0

#define IRQ_PRIORITY_I2C3_DMA_TX			5
#define IRQ_SUB_PRIORITY_I2C3_DMA_TX		0

// ENC28J60
#define IRQ_PRIORITY_SPI2					1
#define IRQ_SUB_PRIORITY_SPI2				0

// Ext
#define IRQ_PRIORITY_SPI3					5
#define IRQ_SUB_PRIORITY_SPI3				0

// ADC
#define IRQ_PRIORITY_ADC					6
#define IRQ_SUB_PRIORITY_ADC				1

//--------------------------------------------------------------------------------------------

#endif
