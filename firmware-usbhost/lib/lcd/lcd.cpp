
#include <stdio.h>
#include <string.h>

#include "stm32f4xx_conf.h"
#include "lcd.h"
#include "common.h"

/*
 * Модуль LCD
 * Тип - динамический
 *
 */

#define	LCD_CONTROL	0x08
#define	LCD_DISPLAY	0x04
#define	LCD_CURSOR	0x02
#define	LCD_BLINK		0x01

#define	LCD_CURSOR_ON		(LCD_CONTROL | LCD_DISPLAY | LCD_CURSOR | LCD_BLINK)
#define	LCD_CURSOR_OFF	(LCD_CONTROL | LCD_DISPLAY)

// Кол-во повторов вывода данных, при ошибочной верификации.
#define REPEAT_IF_ERROR_READ 4

extern void _delay_us(uint32_t us);
extern void _delay_ms(uint32_t ms);


Lcd::Lcd(enum Mode mode, uint32_t rcc_AHB1PeriphCommand, GPIO_TypeDef *commandPort, uint16_t startCommandPin, uint32_t rcc_AHB1PeriphData, GPIO_TypeDef *dataPort, uint16_t startDataPin)
: mode(mode), rcc_AHB1PeriphCommand(rcc_AHB1PeriphCommand), commandPort(commandPort), startCommandPin(startCommandPin), rcc_AHB1PeriphData(rcc_AHB1PeriphData), dataPort(dataPort), startDataPin(startDataPin)
{
	init();
}

void Lcd::init()
{
	if (mode == Mode_4_bit)
		init_4bit();
}

//**************************************************************************
void Lcd::delay()
{
	_delay_us(20);
}
//**************************************************************************
void Lcd::write_4bits(uint8_t data)
{
	int d = data & 0x0F;
	GPIO_ResetBits(dataPort, LCD_DATA_4_PINS(startDataPin));
	GPIO_SetBits(dataPort, d << startDataPin);

	LCD_SET_E;
	delay();
	LCD_CLR_E;
	delay();
}

void Lcd::write_8bits(uint8_t data)
{
	int d = data;
	GPIO_ResetBits(dataPort, LCD_DATA_8_PINS(startDataPin));
	GPIO_SetBits(dataPort, d << startDataPin);

	LCD_SET_E;
	delay();
	LCD_CLR_E;
}

uint8_t Lcd::read_4bits()
{
	uint8_t data;

	LCD_SET_E;
	delay();
	data = GPIO_ReadInputData(dataPort) >> startDataPin;

	LCD_CLR_E;

	return data;
}

void Lcd::write_byte(uint8_t data)
{
	setOutputDataPort();
	if (mode == Mode_4_bit)
	{
		write_4bits(data >> 4);
		write_4bits(data);
	}
	/*
  else if (mode == 8) {
   write_8bits(data);
 } 
	 */
}

uint8_t Lcd::read_byte()
{
	uint8_t result = 0;
	setInputDataPort();

	delay();

	if (mode == Mode_4_bit)
	{
		result = read_4bits() << 4;
		result |= (read_4bits() & 0x0F);
	}
	return result;
}
//**************************************************************************
void Lcd::println(const char* str, uint8_t count)
{
	LCD_SET_RS;             // RS=1, данные

	uint8_t index;
	for(index=0; index<count; index++)
	{
		if (str[index] == 0x00)	write_byte(0x20);
		else											write_byte(str[index]);
	}
}
//**************************************************************************
void Lcd::portsInit()
{
	RCC_AHB1PeriphClockCmd(rcc_AHB1PeriphCommand, ENABLE);
	RCC_AHB1PeriphClockCmd(rcc_AHB1PeriphData, ENABLE);


	if (mode == Mode_4_bit)
	{
		GPIO_InitTypeDef gpio;
		GPIO_StructInit(&gpio);
		gpio.GPIO_OType = GPIO_OType_PP;
		gpio.GPIO_Mode = GPIO_Mode_OUT;
		gpio.GPIO_Pin = LCD_COMMAND_PINS(startCommandPin);
		GPIO_Init(commandPort, &gpio);

		GPIO_ResetBits(commandPort, LCD_COMMAND_PINS(startCommandPin));
	}

	setOutputDataPort();
}

void Lcd::setOutputDataPort()
{
	if (mode == Mode_4_bit)
	{
		GPIO_InitTypeDef gpio;
		GPIO_StructInit(&gpio);
		gpio.GPIO_OType = GPIO_OType_PP;
		gpio.GPIO_Mode = GPIO_Mode_OUT;
		gpio.GPIO_Pin = LCD_DATA_4_PINS(startDataPin);
		GPIO_Init(dataPort, &gpio);
	}	
}

void Lcd::setInputDataPort()
{
	if (mode == Mode_4_bit)
	{
		GPIO_InitTypeDef gpio;
		GPIO_StructInit(&gpio);
		gpio.GPIO_OType = GPIO_OType_PP;
		gpio.GPIO_Mode = GPIO_Mode_IN;
		gpio.GPIO_Pin = LCD_DATA_4_PINS(startDataPin);
		GPIO_Init(dataPort, &gpio);
	}
}

//    Инициализация в 4-Х битном режиме.
void Lcd::init_4bit()
{  
	BYTE us = 45;
	mode = Mode_4_bit;

	portsInit();

	_delay_ms(30);

	write_8bits(0x03);    
	_delay_us(us);
	write_8bits(0x03);   
	_delay_us(us);
	write_8bits(0x03);    
	_delay_us(us);
	write_8bits(0x02);
	_delay_us(us);


	write_byte(0x28);
	_delay_us(us);

	write_byte(0x08);
	_delay_us(us);

	write_byte(0x06);
	_delay_us(us);

	write_byte(0x0f);
	_delay_us(us);

	write_byte(0x01);
	_delay_ms(2);

}

//   Жидкокристаллический модуль МТ-16S2R
//   Инициализация в 8-и битном режиме. По мануалу.
void Lcd::init_8bit()
{
	/*
 mode = Mode_8_bit;

 DDRB |= 0xFF;
 DDRG |= LCD_UPRAVLENIE_B;
 DDRA |= LCD_LIGHT;


 LCD_COMMAND_PORT &= ~(LCD_E | LCD_RS); // E=0 RW=0 RS=0

 write_byte(0x3A);    // 8 bit, Знакогенератор - страница 1.
 delay();
 write_byte(0x3A);    // 8 bit, Знакогенератор - страница 1.
 delay();
 write_byte(0x3A);    // 8 bit, Знакогенератор - страница 1.
 delay();



 write_byte(0x08);    // 8 bit, Выключаем модуль
 delay();

 write_byte(0x0C);    // Включаем модуль
 delay();

 write_byte(0x3A);    // 8 bit, Знакогенератор - страница 1.
 delay();


 write_byte(0x01);    // Clear
 delay();


 write_byte(0x06);    // Направление сдвига вправо
 delay();
	 */
}
//**************************************************************************
void Lcd::set_AC(uint8_t address)
{
	LCD_CLR_E;	LCD_CLR_RS;	LCD_CLR_RW;

	delay();
	write_byte(address | 0x80);						// команда установки адреса DDRAM
	delay();
}

void Lcd::cursorOn() {
	LCD_CLR_E;	LCD_CLR_RS;	LCD_CLR_RW;

	delay();
	write_byte(LCD_CURSOR_ON);						// команда установки адреса DDRAM
	delay();
}

void Lcd::cursorOff() {
	LCD_CLR_E;	LCD_CLR_RS;	LCD_CLR_RW;

	delay();
	write_byte(LCD_CURSOR_OFF);						// команда установки адреса DDRAM
	delay();
}

//**************************************************************************
void Lcd::showValDec(unsigned long Val)     // Вывести десятичное число
{
	if (Val > 999999) { println("max=999999", 10); return; }
	BYTE pV[7];
	int temp;
	int pos = 5;
	do {
		temp = Val /10;
		temp *= 10;
		pV[pos--] = (Val - temp) + '0';
		Val = temp/10;
	} while(Val > 9 && pos);

	pV[pos] = Val + '0';
	pV[6] = 0;

	char *ps = (char *) &(pV[pos]);
	println(ps, 6-pos);
}
//------------------------------------------------------------------------------------
void Lcd::clear()
{
	LCD_CLR_E;	LCD_CLR_RS;	LCD_CLR_RW;

	write_byte(0x01);											//	Clear
	// lcd_write_byte(0x06);											//	Перемещаем курсор в левую позицию
}
//------------------------------------------------------------------------------------

//void Lcd::setDataPin(uint8_t data)
//{
//	GPIO_ResetBits(dataPort, LCD_DATA_8_PINS(startDataPin));
//	GPIO_SetBits(dataPort, data << startDataPin);
//};

//------------------------------------------------------------------------------------

void Lcd::decodeRussian(char *pTxt, BYTE len) {
#define MAX_LEN 64
	char rus[MAX_LEN]	   = {'А', 'Б', 'В', 'Г', 'Д', 'Е', 'Ж', 'З', 'И', 'Й', 'К', 'Л', 'М', 'Н', 'О', 'П', 'Р', 'С', 'Т', 'У', 'Ф', 'Х', 'Ц', 'Ч', 'Ш', 'Щ', 'Ъ', 'Ы', 'Ь', 'Э', 'Ю', 'Я', 'а', 'б', 'в', 'г', 'д', 'е', 'ж', 'з', 'и', 'й', 'к', 'л', 'м', 'н', 'о', 'п', 'р', 'с', 'т', 'у', 'ф', 'х', 'ц', 'ч', 'ш', 'щ', 'ъ', 'ы', 'ь', 'э', 'ю', 'я'};
	uint8_t tablo[MAX_LEN] = {0x41,0xA0,0x42,0xA1,0xE0,0x45,0xA3,0xA4,0xA5,0xA6,0x4B,0xA7,0x4D,0x48,0x4F,0xA8,0x50,0x43,0x54,0xA9,0xAA,0x58,0xE1,0xAB,0xAC,0xE2,0xAD,0xAE,0x62,0xAF,0xB0,0xB1,0x61,0xB2,0xB3,0xB4,0xE3,0x65,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0x6F,0xBE,0x70,0x63,0xBF,0x79,0xE4,0x78,0xE5,0xC0,0xC1,0xe6,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7};

	BYTE x1, x2;
	for(x1 = 0; x1 < len; x1++) {
		for(x2 = 0; x2 < MAX_LEN; x2++) {
			if(pTxt[x1] == rus[x2]) {
				pTxt[x1] = tablo[x2];
				break;
			}
		}
	}
#undef MAX_LEN
}
//------------------------------------------------------------------------------------
/*
void Lcd::showLongTxt(const char *pTxt)
{
  size_t len = strlen(pTxt);
  uint8_t pol = MAX_SIZE_LCD/2;
//  decodeRussian(pTxt, len);
  clear();
  set_AC(LCD_LINE1);
  if (len <= pol) println(pTxt, len);
  else          {
									println(pTxt, pol);
                  set_AC(LCD_LINE2);
									println(&(pTxt[pol]), len - pol);
                }   
}
 */
//------------------------------------------------------------------------------------
void Lcd::setPage(uint8_t page)  // 0-1
{
	if (page > 1) return;

	LCD_CLR_E;	LCD_CLR_RS;
	write_byte(0x38 | (page << 1));
	LCD_SET_RS;			//	RS=1, данные
	delay();
}
//------------------------------------------------------------------------------------

/*
void Lcd::setLight(bool on)
{
  if (on) PORTA |= LCD_LIGHT;
  else    PORTA &= ~LCD_LIGHT;  
}
 */

void Lcd::show(const char *txt) {
	show(txt, Line1);
}

void Lcd::show(const char *txt, enum Lines line) {
	char str[MAX_SIZE_LCD_STR+1];
	uint16_t count = strlen(txt);
	if (count >= MAX_SIZE_LCD_STR) count = MAX_SIZE_LCD_STR;
	strncpy(str, txt, count);

	decodeRussian(str, count);
	set_AC((uint8_t)line);
	println(str, count);

	int index = 0;
	while (!verify(str, count, line) && index++ < REPEAT_IF_ERROR_READ) {
		init();
		set_AC((uint8_t)line);
		println(str, count);	
	}
}

/*
void Lcd::showWithoutDecode(const char *txt, enum Lines line) {
	char str[MAX_SIZE_LCD_STR+1];
	uint16_t i = 0;
	for(; txt[i] != '\0' && i < MAX_SIZE_LCD_STR; i++) {
		str[i] = txt[i];
	}
	set_AC((uint8_t)line);
	println(str, i);	
}
 */
void Lcd::clear(enum Lines line) {
	char str[MAX_SIZE_LCD_STR+1];
	memset(str, ' ', MAX_SIZE_LCD_STR);
	str[MAX_SIZE_LCD_STR] = 0;
	show(str, line);	
}

bool Lcd::verify(const char *txt, uint16_t count, enum Lines line)
{
	set_AC((uint8_t)line);
	LCD_CLR_E;
	LCD_SET_RS; LCD_SET_RW;

	for(uint8_t index = 0; index < count; index++)
	{
		uint8_t b = read_byte();
		if (txt[index] != b) {
			return false;
		}
	}
	return true;
}
