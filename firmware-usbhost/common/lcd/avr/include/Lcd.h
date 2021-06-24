
#ifndef __LCD_H_
#define __LCD_H_

#include <stdint.h>

class Lcd {
	public:
		enum Mode {
			Mode_4_bit,
			Mode_8_bit
		};
		
		enum Lines {
			Line1 = 0,
			Line2 = 0x40,
			Line3 = 0x14,
			Line4 = 0x54
		};		
		
		enum Symbols {
			Up = 0x86,
			Down = 0x87,
			Dinamic = 0x0A,
			Key = 0x19,
			Ok = 0x17,
			Note = 0xBC,
			Mute = 0xBD,
			Volume0 = 0xB3,
			Volume1 = 0xB2,
			SeparateLeft = 0x90,
			SeparateRight = 0x91
		};
	
	private:
		enum Mode mode;
		volatile uint8_t *dataDDR;
		volatile uint8_t *dataPORT;
		volatile uint8_t *dataPIN;
		volatile uint8_t *commandDDR;
		volatile uint8_t *commandPORT;
		uint8_t commandMaskPinRS;
		uint8_t commandMaskPinRW;
		uint8_t commandMaskPinE;
		uint8_t lineSize;			// Длина строки
		uint8_t linesCount;	// Кол-во строк
		
		uint8_t addDelayUs;
		enum Lines lastCursorPosition;
		bool allCentered;
		bool russianSupport;
	
		void portsInit();
		void setOutputDataPort();
		void setInputDataPort();
		inline uint8_t getCommandPorts();
		
		void delay();
		void write_4bits(uint8_t data);
		void write_8bits(uint8_t data);
		void write_byte(uint8_t data);
	
		uint8_t read_4bits();
		uint8_t read_8bits();
		uint8_t read_byte();
		void sendString(const char* line, uint8_t count);
		void init_4bit();
		void init_8bit();
		void setCursorPosition(uint8_t address);
		void showValDec(unsigned long Val);
		void decodeRussian(char *pTxt, uint8_t len);
		void setPage(uint8_t page);
	
	public:	
		Lcd(enum Mode mode, volatile uint8_t *dataDDR, volatile uint8_t *dataPORT, volatile uint8_t *dataPIN, volatile uint8_t *commandDDR, volatile uint8_t *commandPORT, uint8_t commandPinRS, uint8_t commandPinRW, uint8_t commandPinE, uint8_t lineSize, uint8_t linesCounts);
		void init();
		
		void clear();
		void clear(enum Lines line);
		void cursorOn();
		void cursorOff();
		void setCentered(bool centered);
		void setRussianSupport(bool russianSupport);
		uint8_t getLineSize();
		uint8_t getLinesCount();

		void show(const char *txt);
		void show(const char *txt, enum Lines line);
		void show(const char *txt, uint8_t len, enum Lines line);
		void show(const char *txt, enum Lines line, bool centered);
		void show(const char *txt, uint8_t len, enum Lines line, bool centered);

		bool verify(const char *txt, uint8_t len, enum Lines line);

		void operator=(const char *str);
		void operator=(enum Lines lineNumber);
		
		uint8_t getDelayUs();
};


#endif
