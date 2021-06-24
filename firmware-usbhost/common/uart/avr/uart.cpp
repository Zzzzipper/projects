
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include "utils/include/StringBuilder.h"
#include "platform/include/platform.h"
#include "common.h"

#include "include/uart.h"

volatile uint8_t * Uart::UCSRA[] = {&UCSR0A, &UCSR1A, &UCSR2A, &UCSR3A};
volatile uint8_t * Uart::UCSRB[] = {&UCSR0B, &UCSR1B, &UCSR2B, &UCSR3B};
volatile uint8_t * Uart::UCSRC[] = {&UCSR0C, &UCSR1C, &UCSR2C, &UCSR3C};
volatile uint8_t * Uart::UBRRL[] = {&UBRR0L, &UBRR1L, &UBRR2L, &UBRR3L};
volatile uint8_t * Uart::UBRRH[] = {&UBRR0H, &UBRR1H, &UBRR2H, &UBRR3H};
volatile uint8_t * Uart::UDR[]   = {&UDR0, &UDR1, &UDR2, &UDR3};
	
static Uart *pUart[] = {NULL, NULL, NULL, NULL};
	
Uart *Uart::get(UARTS index)
{
	if (index > UART3) index = UART0;
	if (pUart[index]) return pUart[index];

	return new Uart(index, UART_DEFAULT_SPEED, Uart::Parity_None, 0);
}

Uart::Uart(UARTS index, uint32_t speed, Parity parity, uint8_t stop_bit)
: isTransmitting(false)//, transmitEnable(false), receiveEnable(false)
{
	if (index > UART3) index = UART3;
	this->index = index;
	pUart[index] = this;
	mode = Mode_Normal;
	parity = Parity_Unknown;
	bit9Enable = false;
	transmitBuffer = NULL;
	receiveBuffer = NULL;
	receiveHandler = NULL;
	transmitHandler = NULL;
	setTransmitBufferSize(UART_DEFAULT_FIFO_SIZE);
	setReceiveBufferSize(UART_DEFAULT_FIFO_SIZE);
	setup(speed, parity, stop_bit);
} 

Uart *Uart::setup(uint32_t speed, Parity parity, uint8_t stop_bit)
{
	if (speed > UART_MAX_SPEED) speed = UART_MAX_SPEED;
 	long bnd = (F_CPU/16/(speed)) - 1;

  (*UCSRB[index]) &= ~((1 << TXEN0) | (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0));

 setParity(parity);
 
  /* 0,1,2  = один stopbit, 1.5, 2 */
  if (stop_bit < 2 ) {
      (*UCSRC[index]) &= ~(1 << USBS0);
  } else {
      (*UCSRC[index]) |= (1 << USBS0);
  }
       
//  baudsetup[6] = 8; -  8 бит данных в кадре

	(*UBRRL[index]) = LOBYTE(bnd);
	(*UBRRH[index]) = HIBYTE(bnd);
  (*UCSRB[index]) |= ((1 << TXEN0) | (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0));
	
	return this;
}

void Uart::setParity(enum Parity parity)
{
  if (this->parity == parity) return;
	
	uint8_t ucsr = (1 << UCSZ00) | (1 << UCSZ01);

  /* 0,1,2,3,4 = нет паритета, odd, even */
  if (parity == Parity_Even)
  {
	  // Even
	  ucsr = (1 << UPM01) | (0 << UPM00) | (1 << UCSZ00) | (1 << UCSZ01);
	  }  else if (parity == Parity_Odd) {
	  // Odd
	  ucsr = (1 << UPM01) | (1 << UPM00) | (1 << UCSZ00) | (1 << UCSZ01);
  }

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		(*UCSRC[index]) &= ~((1 << UPM01) | (1 << UPM00) | (1 << UCSZ00) | (1 << UCSZ01));
		(*UCSRC[index]) |= ucsr;
		this->parity = parity;
	}
}

void Uart::setMode(enum Mode mode)
{
	if (this->mode == mode) return;
	this->mode = mode;
	if (mode == Mode_9Bit) {
		setParity(Parity_None);
		(*UCSRB[index]) |= (1 << UCSZ02);
	} else {
		(*UCSRB[index]) &= ~(1 << UCSZ02);
	}
}

void Uart::set9Bit(const uint8_t byte)
{
	(*UCSRB[index]) |= MASK(TXB80);
}

void Uart::clear9Bit(const uint8_t byte)
{
	(*UCSRB[index]) &= ~MASK(TXB80);
}

void Uart::setTransmitBufferSize(uint16_t size)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (transmitBuffer) delete transmitBuffer;
		transmitBuffer = new Fifo<uint8_t>(size);
	}
}

void Uart::setReceiveBufferSize(uint16_t size)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (receiveBuffer) delete receiveBuffer;
		receiveBuffer = new Fifo<uint8_t>(size);
	}
}

void Uart::send(uint8_t b)
{
	while (this->isFullTransmitBuffer()) startTransmit();
	transmitBuffer->push(b);
	startTransmit();
}

uint8_t Uart::receive()
{
	return receiveBuffer->pop();
}

bool Uart::isEmptyReceiveBuffer()
{
	return receiveBuffer->isEmpty();
}

bool Uart::isFullTransmitBuffer()
{
	return transmitBuffer->isFull();
}

void Uart::setReceiveHandler(UartReceiveHandler *handler)
{
	receiveHandler = handler;
}

void Uart::setTransmitHandler(UartTransmitHandler *handler)
{
	transmitHandler = handler;
}

void Uart::execute() {
	if(receiveHandler) {
		if(receiveBuffer->getSize() >= receiveHandler->getLen()) {
			receiveHandler->handle();
		}
	}
	
	if(transmitHandler) {
		if(transmitBuffer->isEmpty()) {
			transmitHandler->emptyTransmitBuffer();
		}
	}
}

void Uart::transmit_isr()
{
	if (!isTransmitting) return;

	if (mode == Mode_9Bit) {
		transmit9Bit_isr();
		return;
	}

	if (transmitBuffer->isEmpty()) {
		stopTransmit();
		return;
	}
	
	(*UDR[index]) = transmitBuffer->pop();
}

void Uart::transmit9Bit_isr()
{ 
	if (transmitBuffer->isEmpty() || transmitBuffer->getSize() < 2) {
		stopTransmit();
		return;
	}
	
	uint8_t setting = transmitBuffer->pop();
	uint8_t data = transmitBuffer->pop();
	
	if (setting & 0x01) set9Bit(data);
	else								clear9Bit(data);
	
	(*UDR[index]) = data;
}

void Uart::receive_isr(const uint8_t ucsrb, const uint8_t data)
{
	if (mode == Mode_9Bit) {
		receive9Bit_isr(ucsrb, data);
		return;
	}

	receiveBuffer->push(data);
}

void Uart::receive9Bit_isr(const uint8_t ucsrb, const uint8_t data)
{
	uint8_t setting = (ucsrb & MASK(RXB80)) ? 1 : 0;

	receiveBuffer->push(setting);
	receiveBuffer->push(data);	
}

void Uart::startTransmit()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Если процесс передачи данных уже запущен, выходим.
		if (isTransmitting) return;
	
		// Если данных для отправки нет, выходим.
		if (transmitBuffer->isEmpty()) return;
		isTransmitting = true;
		
		(*UCSRB[index]) |= ((1 << TXEN0) | (1 << TXCIE0));
		transmit_isr();
	}
}

void Uart::stopTransmit()
{
	isTransmitting = false;
}

void Uart::enableReceive()
{
	(*UCSRB[index]) |= ((1 << RXEN0) | (1 << RXCIE0));
}

void Uart::disableReceive()
{
	(*UCSRB[index]) &= ~((1 << RXEN0) | (1 << RXCIE0));
}

bool Uart::isTransmitEmpty()
{
	return (*UCSRA[index]) & MASK(UDRE0);
}

void Uart::send(const uint8_t *p, uint16_t len)
{
	for (uint16_t i = 0; i < len; i++)
	{
		send(p[i]);
	}
}

void Uart::send(StringBuilder &str)
{
	send((uint8_t *)str.getString(), str.getLen());
}

void Uart::send(const char *str)
{
	char *p = (char *) str;
	while (*p) send(*p++);
}

void Uart::sendln(const char *str)
{
	send(str);
	send("\r\n");
}

void Uart::clear()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		transmitBuffer->clear();
		receiveBuffer->clear();
	}
}

void Uart::disable()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		transmitBuffer->clear();
		receiveBuffer->clear();
		stopTransmit();
		(*UCSRB[index]) = 0;
	}
}

//! Пришел 1 байт по UART0
ISR(USART0_RX_vect)
{
	uint8_t a = UCSR0B;
	uint8_t b = UDR0;
	if (pUart[UART0]) pUart[UART0]->receive_isr(a, b);
}

//! 1 байт по UART0 передан
ISR(USART0_TX_vect)
{
	if (pUart[UART0]) pUart[UART0]->transmit_isr();
}

ISR(USART1_RX_vect)
{
	uint8_t a = UCSR1B;
	uint8_t b = UDR1;
	if (pUart[UART1]) pUart[UART1]->receive_isr(a, b);
}

ISR(USART1_TX_vect)
{
	if (pUart[UART1]) pUart[UART1]->transmit_isr();
}

ISR(USART2_RX_vect)
{
	uint8_t a = UCSR2B;
	uint8_t b = UDR2;
	if (pUart[UART2]) pUart[UART2]->receive_isr(a, b);
}

ISR(USART2_TX_vect)
{
	if (pUart[UART2]) pUart[UART2]->transmit_isr();
}

ISR(USART3_RX_vect)
{
	uint8_t a = UCSR3B;
	uint8_t b = UDR3;
	if (pUart[UART3]) pUart[UART3]->receive_isr(a, b);
}

ISR(USART3_TX_vect)
{
	if (pUart[UART3]) pUart[UART3]->transmit_isr();
}
