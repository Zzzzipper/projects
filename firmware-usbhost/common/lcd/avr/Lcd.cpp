/*
	Модуль работы с LCD экраном, версия для AVR
	Пример вызова конструктора:
	
	В 4-х битном режиме:
	Lcd *lcd = new Lcd(Lcd::Mode_4_bit, &DDRC, &PORTC, &PINC, &DDRG, &PORTG, DDG1, DDG0, DDG2, 16, 2);
	
	В 4-х битном режиме, без проверки через чтение:
	Lcd *lcd = new Lcd(Lcd::Mode_4_bit, &DDRC, &PORTC, &PINC, &DDRG, &PORTG, DDG1, 0xff, DDG2, 16, 2);
*/


#include <avr/io.h>
#include <avr/interrupt.h>

#include <avr/wdt.h>
#include <stdio.h>
#include <util/delay.h>
#include <string.h>

#include <common.h>
#include "include/Lcd.h"

#define	LCD_CONTROL	0x08
#define	LCD_DISPLAY	0x04
#define	LCD_CURSOR	0x02
#define	LCD_BLINK	0x01

#define MAX_LINE_SIZE 32

#define	LCD_CURSOR_ON	(LCD_CONTROL | LCD_DISPLAY | LCD_CURSOR | LCD_BLINK)
#define	LCD_CURSOR_OFF	(LCD_CONTROL | LCD_DISPLAY)

// Кол-во повторов вывода данных, при ошибочной верификации.
#define REPEAT_IF_ERROR_READ 10
#define DELAY_START_VALUE_MS 20
#define DELAY_STEP_MS 4
#define DELAY_MAX_VALUE_MS 50


// mode = 4 = 4 bit, 8 = 8 bit

#define NONE 0xff

Lcd::Lcd(enum Mode mode, volatile uint8_t *dataDDR, volatile uint8_t *dataPORT, volatile uint8_t *dataPIN, volatile uint8_t *commandDDR, volatile uint8_t *commandPORT, uint8_t commandPinRS, uint8_t commandPinRW, uint8_t commandPinE, uint8_t lineSize, uint8_t lineCounts)
: mode(mode), dataDDR(dataDDR), dataPORT(dataPORT), dataPIN(dataPIN), commandDDR(commandDDR), commandPORT(commandPORT), commandMaskPinRS(MASK(commandPinRS)), commandMaskPinE(MASK(commandPinE)), lineSize(lineSize), linesCount(lineCounts)
{
	if (commandPinRW == NONE) this->commandMaskPinRW = NONE;
	else this->commandMaskPinRW = MASK(commandPinRW);
	
	addDelayUs = 0;
	allCentered = false;
	russianSupport = true;
	init();
}

void Lcd::init()
{
	if (mode == Mode_4_bit)
		init_4bit();
	else
		init_8bit();
}

//**************************************************************************
void Lcd::delay()
{
	uint8_t d = getDelayUs();
	while (d--)	_delay_us(1);      
}
//**************************************************************************
void Lcd::write_4bits(uint8_t data)
{
	data &= 0x0F;
	(*dataPORT) &= ~0x0F;
	(*dataPORT) |= data;
	(*commandPORT) |= commandMaskPinE;
	delay();
	(*commandPORT) &= ~commandMaskPinE;
}

void Lcd::write_8bits(uint8_t data)
{
	(*dataPORT) = data;
	(*commandPORT) |= commandMaskPinE;
	delay();
	(*commandPORT) &= ~commandMaskPinE;
}

uint8_t Lcd::read_4bits()
{
	uint8_t data;
	
	(*commandPORT) |= commandMaskPinE;
	delay();
	data = (*dataPIN);
	(*commandPORT) &= ~commandMaskPinE;
	return data;
}

uint8_t Lcd::read_8bits()
{
	uint8_t data;
	
	(*commandPORT) |= commandMaskPinE;
	delay();
	data = (*dataPIN);
	(*commandPORT) &= ~commandMaskPinE;
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
  else {
   write_8bits(data);
 } 
  
}

uint8_t Lcd::read_byte()
{
	uint8_t result = 0;
	setInputDataPort();
	
	delay();
		
	if (mode == Mode_4_bit)	{
		result = read_4bits() << 4;
		result |= (read_4bits() & 0x0F);
	} else {
		result = read_8bits();		
	}
	return result;
}
 //**************************************************************************
 void Lcd::sendString(const char* str, uint8_t count)
 {
   (*commandPORT) |= commandMaskPinRS;             // RS=1, данные 

  uint8_t index;
  for(index = 0; index < count; index++)
  {
   if (str[index] == 0x00)	write_byte(0x20);
	 else											write_byte(str[index]);
  } 
 }
//**************************************************************************
void Lcd::portsInit()
{
	setOutputDataPort();

	(*commandDDR) |= getCommandPorts();
	(*commandPORT) &= ~getCommandPorts(); // E=0 RW=0 RS=0
}

void Lcd::setOutputDataPort()
{
	if (mode == Mode_4_bit)
		(*dataDDR) |= 0x0F;	
	else
		(*dataDDR) |= 0xFF;
}

void Lcd::setInputDataPort()
{
	if (mode == Mode_4_bit)
		(*dataDDR) &= ~0x0F;
	else 
		(*dataDDR) &= ~0xFF;
}

//    Инициализация в 4-Х битном режиме.
void Lcd::init_4bit()
{  
	uint8_t us = 45;
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
	uint8_t us = 45;
	mode = Mode_8_bit;
	portsInit();
	
	_delay_ms(30);

	write_byte(0x3A);    // 8 bit, Знакогенератор - страница 1.
	_delay_us(us);
	write_byte(0x3A);    // 8 bit, Знакогенератор - страница 1.
	_delay_us(us);
	write_byte(0x3A);    // 8 bit, Знакогенератор - страница 1.
	_delay_us(us);

	write_byte(0x08);    // 8 bit, Выключаем модуль
	_delay_us(us);

	write_byte(0x0C);    // Включаем модуль
	_delay_us(us);

	write_byte(0x3A);    // 8 bit, Знакогенератор - страница 1.
	_delay_us(us);

	write_byte(0x01);    // Clear
	_delay_ms(2);

	write_byte(0x06);    // Направление сдвига вправо
	_delay_us(us);
}
//**************************************************************************
void Lcd::setCursorPosition(uint8_t address)
{
 (*commandPORT) &= ~(getCommandPorts());	// E=0 RW=0 RS=0
 write_byte(address | 0x80);						// команда установки адреса DDRAM 
 delay();
}

void Lcd::cursorOn() {
	(*commandPORT) &= ~(getCommandPorts());	// E=0 RW=0 RS=0
	write_byte(LCD_CURSOR_ON);						// команда установки адреса DDRAM
	delay();
}

void Lcd::cursorOff() {
	(*commandPORT) &= ~(getCommandPorts());	// E=0 RW=0 RS=0
	write_byte(LCD_CURSOR_OFF);						// команда установки адреса DDRAM
	delay();
}

//**************************************************************************
void Lcd::showValDec(unsigned long Val)     // Вывести десятичное число
{
 if (Val > 999999) { sendString("max=999999", 10); return; }
 uint8_t pV[7];
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
 sendString(ps, 6-pos);
}
//------------------------------------------------------------------------------------
void Lcd::clear()
{
 (*commandPORT) &= ~(getCommandPorts()); //	E=0 RW=0 RS=0
 write_byte(0x01);											//	Clear
 // lcd_write_byte(0x06);											//	Перемещаем курсор в левую позицию
}
//------------------------------------------------------------------------------------

void Lcd::decodeRussian(char *pTxt, uint8_t len) {
	if (!russianSupport) return;
	#define MAX_LEN 64
	char rus[MAX_LEN]	   = {'А', 'Б', 'В', 'Г', 'Д', 'Е', 'Ж', 'З', 'И', 'Й', 'К', 'Л', 'М', 'Н', 'О', 'П', 'Р', 'С', 'Т', 'У', 'Ф', 'Х', 'Ц', 'Ч', 'Ш', 'Щ', 'Ъ', 'Ы', 'Ь', 'Э', 'Ю', 'Я', 'а', 'б', 'в', 'г', 'д', 'е', 'ж', 'з', 'и', 'й', 'к', 'л', 'м', 'н', 'о', 'п', 'р', 'с', 'т', 'у', 'ф', 'х', 'ц', 'ч', 'ш', 'щ', 'ъ', 'ы', 'ь', 'э', 'ю', 'я'};
	uint8_t tablo[MAX_LEN] = {0x41,0xA0,0x42,0xA1,0xE0,0x45,0xA3,0xA4,0xA5,0xA6,0x4B,0xA7,0x4D,0x48,0x4F,0xA8,0x50,0x43,0x54,0xA9,0xAA,0x58,0xE1,0xAB,0xAC,0xE2,0xAD,0xAE,0x62,0xAF,0xB0,0xB1,0x61,0xB2,0xB3,0xB4,0xE3,0x65,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0x6F,0xBE,0x70,0x63,0xBF,0x79,0xE4,0x78,0xE5,0xC0,0xC1,0xe6,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7};

	uint8_t x1, x2;
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

	(*commandPORT) &= ~(getCommandPorts());    //	E=0 RW=0 RS=0
	write_byte(0x38 | (page << 1));			//
	(*commandPORT) |= commandMaskPinRS;			//	RS=1, данные 
	delay();
}
//------------------------------------------------------------------------------------

void Lcd::show(const char *txt)
{
	uint16_t len = strlen(txt);
	show(txt, len, lastCursorPosition, allCentered);
}

void Lcd::show(const char *txt, enum Lines line)
{
	uint16_t len = strlen(txt);
	show(txt, len, line, allCentered);
}

void Lcd::show(const char *txt, uint8_t len, enum Lines line)
{
	show(txt, len, line, allCentered);
}

void Lcd::show(const char *txt, enum Lines line, bool centered)
{
	uint16_t len = strlen(txt);
	show(txt, len, line, centered);
}

void Lcd::show(const char *txt, uint8_t len, enum Lines line, bool centered)
{
	// TODO: добавить параметр или функцию перерисовки всей строки, за текстом пробелами
	
	char str[MAX_LINE_SIZE + 1];
	if (len >= lineSize) len = lineSize;
	
	if (centered) {
		uint8_t spaces = lineSize - len;
		uint8_t leftSp = spaces / 2;
		memset(str, ' ', lineSize);
		strncpy(&(str[leftSp]), txt, len);
		len += spaces;
	}
	else
		strncpy(str, txt, len);
	
	decodeRussian(str, len);

	lastCursorPosition = line;
	setCursorPosition((uint8_t)line);

	sendString(str, len);
	
	int index = 0;
	if (commandMaskPinRW != NONE)
	{
		while (!verify(str, len, lastCursorPosition) && index++ < REPEAT_IF_ERROR_READ)
		{
			if (addDelayUs < DELAY_MAX_VALUE_MS)	addDelayUs += DELAY_STEP_MS;
			init();
			setCursorPosition((uint8_t) lastCursorPosition);
			sendString(str, len);
		}
	}
}


void Lcd::clear(enum Lines line)
{
	char str[MAX_LINE_SIZE + 1];
	memset(str, ' ', lineSize);
	str[lineSize] = 0;
	show(str, line);	
}

bool Lcd::verify(const char *txt, uint8_t len, enum Lines line)
{
	setCursorPosition((uint8_t)line);
	(*commandPORT) &= ~(commandMaskPinE);
	if (commandMaskPinRW == -1) (*commandPORT) |= (commandMaskPinRS);	// E=0 RS=1
	else 												(*commandPORT) |= (commandMaskPinRS | commandMaskPinRW);	// E=0 RW=1 RS=1

	for(uint8_t index = 0; index < len; index++)
	{
		uint8_t b = read_byte();
		if (txt[index] != b) {
		return false;
	}
 }
	return true;
}

uint8_t Lcd::getCommandPorts()
{
	if (commandMaskPinRW == -1) return commandMaskPinRS | commandMaskPinE;
	return commandMaskPinRS | commandMaskPinRW | commandMaskPinE;
}

uint8_t Lcd::getDelayUs()
{
	return DELAY_START_VALUE_MS + addDelayUs;	
}

void Lcd::setCentered(bool centered)
{
	this->allCentered = centered;
}

void Lcd::setRussianSupport(bool russianSupport)
{
	this->russianSupport = russianSupport;
}


void Lcd::operator=(const char *str)
{
	show(str);
}

void Lcd::operator=(enum Lines lineNumber)
{
	lastCursorPosition = lineNumber;
	setCursorPosition((uint8_t)lineNumber);
}

uint8_t Lcd::getLineSize()
{
	return lineSize;	
}

uint8_t Lcd::getLinesCount()
{
	return linesCount;
}
