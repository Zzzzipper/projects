
#ifndef __LCD_H_
#define __LCD_H_

#include <stdint.h>
#include "common/utils/include/StringBuilder.h"
#include "common/common.h"

enum lcd_union_special_symbol {
	sp_UP=0x86,
	sp_DOWN=0x87,
	sp_DINAMIC=0x0A, 
	sp_KEY=0x19, 
	sp_OK=0x17, 
	sp_NOTE=0xBC, 
	sp_MUTE=0xBD, 
	sp_Volume_0 = 0xB3, 
	sp_Volume_1 = 0xB2, 
	sp_SeparateLeft = 0x90, 
	sp_SeparateRight = 0x91
};
 
 //enum display_configuration
/* 
enum lcd_effects {
	ef_default, 
	ef_ToLeft, 
	ef_ToRight
};
*/

#define MAX_SIZE_LCD					80														// Макс. кол-во символов на экране
#define LCD_STR_NUM						4															// Кол-во строк
#define MAX_SIZE_LCD_STR			(MAX_SIZE_LCD/LCD_STR_NUM)		// Размер одной строки


#define LCD_RS(x)						(x << 0)
#define LCD_RW(x)						(x << 1)
#define LCD_E(x)						(x << 2)
#define LCD_COMMAND_PINS(x) (LCD_RS(x) | LCD_RW(x) | LCD_E(x))

//#define LCD_LIGHT  MASK(DDA7)


#define LCD_D0(x)						(x << 0)
#define LCD_D1(x)						(x << 1)
#define LCD_D2(x)						(x << 2)
#define LCD_D3(x)						(x << 3)
#define LCD_D4(x)						(x << 4)
#define LCD_D5(x)						(x << 5)
#define LCD_D6(x)						(x << 6)
#define LCD_D7(x)						(x << 7)
#define LCD_DATA_4_PINS(x)	(LCD_D0(x) | LCD_D1(x) | LCD_D2(x) | LCD_D3(x))
#define LCD_DATA_8_PINS(x)	(LCD_D0(x) | LCD_D1(x) | LCD_D2(x) | LCD_D3(x) | LCD_D4(x) | LCD_D5(x) | LCD_D6(x) | LCD_D7(x))

#define LCD_SET_E						GPIO_SetBits(commandPort, LCD_E(startCommandPin))
#define LCD_CLR_E						GPIO_ResetBits(commandPort, LCD_E(startCommandPin))
#define LCD_SET_RS					GPIO_SetBits(commandPort, LCD_RS(startCommandPin))
#define LCD_CLR_RS					GPIO_ResetBits(commandPort, LCD_RS(startCommandPin))
#define LCD_SET_RW					GPIO_SetBits(commandPort, LCD_RW(startCommandPin))
#define LCD_CLR_RW					GPIO_ResetBits(commandPort, LCD_RW(startCommandPin))


//Требования к API
//1. чтобы строка переписывалась до конца строки
//2. чтобы курсор можно было включать и выключать на строке
//3. чтобы была возможность сделать строку бегущей по требованию и выводить в ней больше символов чем влезает в строку

class Lcd {
	public:
		enum Mode {
			Mode_4_bit
		};
		
		enum Lines {
			Line1 = 0,
			Line2 = 0x40,
			Line3 = 0x14,
			Line4 = 0x54
		};		
	
	private:
		enum Mode mode;
		uint32_t rcc_AHB1PeriphCommand;
		GPIO_TypeDef *commandPort;
		uint16_t startCommandPin;
		uint32_t rcc_AHB1PeriphData;
		GPIO_TypeDef *dataPort;
		uint16_t startDataPin;
	
		void portsInit();
		void setOutputDataPort();
		void setInputDataPort();
		
		void delay();
		void write_4bits(uint8_t data);
		void write_8bits(uint8_t data);
		void write_byte(uint8_t data);
//		void setDataPin(uint8_t data);
	
		uint8_t read_4bits();
		uint8_t read_byte();
		void println(const char* line, uint8_t count);
		void init_4bit();
		void init_8bit();
		void set_AC(uint8_t address);
		void showValDec(unsigned long Val);
		void decodeRussian(char *pTxt, BYTE len);
		void setPage(uint8_t page);
	//		void showLongTxt(const char *txt);
	
	public:	
		//! Пример инициализации: new Lcd(Lcd::Mode_4_bit, RCC_AHB1Periph_GPIOA, GPIOA, 0, RCC_AHB1Periph_GPIOA, GPIOA, 5);
		//! /param mode - режим 4-х или 8-и битный
		//! /param rcc_AHB1PeriphCommand - Включает тактовую частоту порта периферии AHB1, к которой будут подключены пины команд
		//! /param commandPort - Порт, к которой будут подключены пины команд
		//! /param startCommandPin - Номер первого пина команд. Все остальные должны быть подключены подряд.
		//! /param rcc_AHB1PeriphData - Включает тактовую частоту порта периферии AHB1, к которой будут подключены пины данных
		//! /param commandData - Порт, к которой будут подключены пины данных
		//! /param startDataPin - Номер первого пина данных. Все остальные должны быть подключены подряд.
		Lcd(enum Mode mode, uint32_t rcc_AHB1PeriphCommand, GPIO_TypeDef *commandPort, uint16_t startCommandPin, uint32_t rcc_AHB1PeriphData, GPIO_TypeDef *dataPort, uint16_t startDataPin);
		void init();
		
		void clear();
		void clear(enum Lines line);
		void cursorOn();
		void cursorOff();

		void show(const char *txt);
		void show(const char *txt, enum Lines line);
//		void showWithoutDecode(const char *txt, enum Lines line);
		bool verify(const char *txt, uint16_t count, enum Lines line);
	
	
};


#endif
