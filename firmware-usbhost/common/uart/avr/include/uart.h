#ifndef __UART2_H
#define __UART2_H

#include "uart/include/interface.h"
#include "utils/include/Fifo.h"

#include <stdint.h>

#define UART_MAX_SPEED			115200
#define UART_DEFAULT_SPEED		9600
#define UART_DEFAULT_FIFO_SIZE	32

enum UARTS {
	UART0,
	UART1,
	UART2,
	UART3
};

class StringBuilder;

class Uart : public AbstractUart {	
public:
	enum Parity {
		Parity_Unknown = -1,
		Parity_None = 0,
		Parity_Odd = 1,
		Parity_Even = 2
	};
	
	enum Mode {
		Mode_Normal,
		Mode_9Bit
	};
	
	static Uart *get(UARTS index);

	//! Настройка параметров УАРТ.
	//! \param speed - скорость, бит/сек
	//! \param parity - четность. 0,1,2,3,4 =  нет паритета, odd,even,mark,space
	//! \param stop_bit - кол-во стоповых бит. 0,1,2 = один stopbit, 1.5, 2
	Uart *setup(uint32_t speed, Parity parity, uint8_t stop_bit);
	void setMode(Mode mode);
	void setTransmitHandler(UartTransmitHandler *handler);
	void setTransmitBufferSize(uint16_t size);
	void setReceiveHandler(UartReceiveHandler *handler);
	void setReceiveBufferSize(uint16_t size);

	void execute();
	void send(uint8_t b);
	uint8_t receive();
	bool isEmptyReceiveBuffer();	
	bool isFullTransmitBuffer();
	void clear();

	void transmit_isr();
	void receive_isr(const uint8_t ucsrb, const uint8_t data);

//+++ Вынести эти методы в отдельный класс. Например класс UartTerminal (намек на работу со строками).
	void send(const uint8_t *p, uint16_t len);
	void send(StringBuilder &str);
	void send(const char *str);
	void sendln(const char *str);
//+++

protected:
	//! Номер UART
	uint8_t index;
	Fifo<uint8_t> *transmitBuffer;
	Fifo<uint8_t> *receiveBuffer;
	volatile bool isTransmitting;

	volatile static uint8_t * UCSRA[];
	volatile static uint8_t * UCSRB[];
	volatile static uint8_t * UCSRC[];
	volatile static uint8_t * UBRRL[];
	volatile static uint8_t * UBRRH[];
	volatile static uint8_t * UDR[];

	Uart(UARTS index, uint32_t speed, Parity parity, uint8_t stop_bit);
	void startTransmit();
	void stopTransmit();
	inline bool isTransmitEmpty();

private:
	Parity parity;
	void setParity(enum Parity parity);

	UartReceiveHandler *receiveHandler;
	UartTransmitHandler *transmitHandler;
	Mode mode;

	bool bit9Enable;
	void set9Bit(const uint8_t byte);
	void clear9Bit(const uint8_t byte);
	
	inline void enableReceive();
	inline void disableReceive();
	
	void transmit9Bit_isr();
	void receive9Bit_isr(const uint8_t ucsrb, const uint8_t data);
	// Отключаем UART от периферии. Это позволит управлять ногами, занятыми UART. Для инициализации приема неоходимо вызвать enableReceive();
	void disable();	
};

#endif
