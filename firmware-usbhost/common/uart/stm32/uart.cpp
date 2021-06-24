#include <stdio.h>
#include <string.h>

#include "uart/stm32/include/uart.h"
#include "config.h"

extern void _delay_ms(uint32_t ms);

static Uart *pUart[] = {NULL, NULL, NULL, NULL, NULL, NULL};

/** USART1 GPIO Configuration
 PA9   ------> USART1_TX
 PA10   ------> USART1_RX
 */

/** USART2 GPIO Configuration
 PA2   ------> USART2_TX
 PA3   ------> USART2_RX
 */

/** USART3 GPIO Configuration
 PB10   ------> USART3_TX
 PB11   ------> USART3_RX
 */

/** USART4 GPIO Configuration
 PC10   ------> USART4_TX
 PC11   ------> USART4_RX
 */

/** USART5 GPIO Configuration
 PC12   ------> USART5_TX
 PD2    ------> USART5_RX
 */

/** USART6 GPIO Configuration
 PC6   ------> USART6_TX
 PC7   ------> USART6_RX
 */

typedef GPIO_TypeDef *	P_GPIO;
typedef USART_TypeDef*	P_USART_TypeDef;

const P_GPIO 	UART_TX_PORT[]	= {GPIOA,		GPIOA,      GPIOB,		 GPIOC,			GPIOC,		GPIOC};
const uint32_t	UART_TX_PIN_SOURCE[] 	= {GPIO_PinSource9, GPIO_PinSource2,  GPIO_PinSource10, GPIO_PinSource10, 	GPIO_PinSource12, GPIO_PinSource6};

const P_GPIO 	UART_RX_PORT[]	= {GPIOA,		GPIOA,		GPIOB,		 GPIOC, 		GPIOD, 		GPIOC};
const uint32_t	UART_RX_PIN_SOURCE[] 	= {GPIO_PinSource10, GPIO_PinSource3, GPIO_PinSource11, GPIO_PinSource11, 	GPIO_PinSource2, GPIO_PinSource7};

const uint8_t	UART_GPIO_AF[]	= {GPIO_AF_USART1, GPIO_AF_USART2, GPIO_AF_USART3, GPIO_AF_UART4, GPIO_AF_UART5, GPIO_AF_USART6};

const IRQn_Type	UART_IRQ[]		= {USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn, USART6_IRQn};

const P_USART_TypeDef UART_TYPEDEF[] = {USART1, USART2, USART3, UART4, UART5, USART6};

const uint32_t USART_IRQ_PRIORITY[] = {IRQ_PRIORITY_USART1, IRQ_PRIORITY_USART2, IRQ_PRIORITY_USART3, IRQ_PRIORITY_UART4, IRQ_PRIORITY_UART5, IRQ_PRIORITY_USART6};

const uint32_t USART_IRQ_SUB_PRIORITY[] = {IRQ_SUB_PRIORITY_USART1, IRQ_SUB_PRIORITY_USART2, IRQ_SUB_PRIORITY_USART3, IRQ_SUB_PRIORITY_UART4, IRQ_SUB_PRIORITY_UART5, IRQ_SUB_PRIORITY_USART6};



Uart *Uart::get(UARTS index) {
	if(pUart[index]) { return pUart[index]; }
	return new Uart(index, UART_DEFAULT_SPEED, Uart::Parity_None, 0);
}

Uart::Uart(UARTS index, uint32_t speed, Parity parity, uint8_t stop_bit) :
	usart(NULL),
	receiveHandler(NULL),
	transmitHandler(NULL),
	mode(Mode_Normal),
	fastForwardingUart(NULL),
	receiveBuffer(NULL),
	transmitBuffer(NULL),
	isTransmitting(false)
{
	this->index = index;
	pUart[index] = this;

	setTransmitBufferSize(UART_DEFAULT_FIFO_SIZE);
	setReceiveBufferSize(UART_DEFAULT_FIFO_SIZE);
	setup(speed, parity, stop_bit);
}

Uart *Uart::setup(uint32_t speed, Parity parity, uint8_t stop_bit)
{
	return setup(speed, parity, stop_bit, Mode::Mode_Normal, UART_DEFAULT_OTYPE);
}

Uart *Uart::setup(uint32_t speed, Parity parity, uint8_t stop_bit, Mode mode)
{
	return setup(speed, parity, stop_bit, mode, UART_DEFAULT_OTYPE);
}

Uart *Uart::setup(uint32_t speed, Parity parity, uint8_t stop_bit, Mode mode, GPIOOType_TypeDef output_type)
{
	this->output_type = output_type;

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	if (speed > UART_MAX_SPEED) speed = UART_MAX_SPEED;

	this->mode = mode;

	if (usart)
	{
		disableReceive();
		USART_Cmd(usart, DISABLE);
	}
	else
		usart = UART_TYPEDEF[index];

	NVIC_DisableIRQ (UART_IRQ[index]);

	switch ((int) index)
	{
	case UART_1:
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	break;

	case UART_2:
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	break;

	case UART_3:
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	break;

	case UART_4:
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
	break;

	case UART_5:
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
	break;

	case UART_6:
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
	break;
	}

	initGpio();

	// TODO: NVIC, USART1
	NVIC_InitStructure.NVIC_IRQChannel = UART_IRQ[index];
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USART_IRQ_PRIORITY[index];
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = USART_IRQ_SUB_PRIORITY[index];
	NVIC_Init(&NVIC_InitStructure);


	USART_StructInit(&usart_InitStructure);

	if (mode == Mode_9Bit)
		usart_InitStructure.USART_WordLength = USART_WordLength_9b;
	else
		usart_InitStructure.USART_WordLength = USART_WordLength_8b;

	usart_InitStructure.USART_BaudRate = speed;
	usart_InitStructure.USART_StopBits = USART_StopBits_1;
	usart_InitStructure.USART_Parity = parity;
	usart_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_DeInit(usart);
	USART_Init(usart, &usart_InitStructure);

	// Включаем прерывание по окончанию передачи
	USART_ClearITPendingBit(usart, USART_IT_TC);
	USART_ITConfig(usart, USART_IT_TC, ENABLE);

	transmitEnable = true;

	// Включаем прерывание по приему данных
	enableReceive();

	// Запускаем USART
	USART_Cmd(usart, ENABLE);

	return this;
}

void Uart::initGpio()
{
	/*Enable or disable the APB, AHB peripheral clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_StructInit(&gpio_InitStructure);

	/*Configure RX pin */

	gpio_InitStructure.GPIO_Pin = (1 << UART_RX_PIN_SOURCE[index]);
	gpio_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	gpio_InitStructure.GPIO_OType = output_type;
	gpio_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	gpio_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(UART_RX_PORT[index], &gpio_InitStructure);

	/*Configure TX pin */

	gpio_InitStructure.GPIO_Pin = (1 << UART_TX_PIN_SOURCE[index]);
	GPIO_Init(UART_TX_PORT[index], &gpio_InitStructure);

	/*Configure GPIO pin alternate function */
	GPIO_PinAFConfig(UART_RX_PORT[index], UART_RX_PIN_SOURCE[index], UART_GPIO_AF[index]);

	/*Configure GPIO pin alternate function */
	GPIO_PinAFConfig(UART_TX_PORT[index], UART_TX_PIN_SOURCE[index], UART_GPIO_AF[index]);
}

GPIO_TypeDef *Uart::getTxPort()
{
	return UART_TX_PORT[index];
}

uint32_t Uart::getTxPinSource()
{
	return UART_TX_PIN_SOURCE[index];
}

GPIOOType_TypeDef Uart::getTxOutputType()
{
	return output_type;
}

void Uart::setParity(const Parity parity)
{
	usart_InitStructure.USART_Parity = parity;
	USART_Init(usart, &usart_InitStructure);
}

void Uart::setTransmitBufferSize(uint32_t bufSize)
{
	ATOMIC {
		if(transmitBuffer) delete transmitBuffer;
		transmitBuffer = new Fifo<uint8_t>(bufSize);
	}
}

void Uart::setTransmitBuffer(uint8_t *buf, uint32_t bufSize)
{
	ATOMIC {
		if(transmitBuffer) delete transmitBuffer;
		transmitBuffer = new Fifo<uint8_t>(bufSize, buf);
	}
}

void Uart::setReceiveBufferSize(uint32_t size)
{
	ATOMIC {
		if(receiveBuffer) delete receiveBuffer;
		receiveBuffer = new Fifo<uint8_t>(size);
	}
}

void Uart::setReceiveBuffer(uint8_t *buf, uint32_t bufSize)
{
	ATOMIC {
		if(receiveBuffer) delete receiveBuffer;
		receiveBuffer = new Fifo<uint8_t>(bufSize, buf);
	}
}

void Uart::send(uint8_t b)
{
	if (!transmitEnable) return;

	while(this->isFullTransmitBuffer()) {
		startTransmit();
		_delay_ms(5);
	}
	transmitBuffer->push(b);
	startTransmit();
}

void Uart::sendAsync(uint8_t b)
{
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

bool Uart::isEmptyTransmitBuffer()
{
	return transmitBuffer->isEmpty();
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
#if 1
	if(receiveHandler && !receiveHandler->isInterruptMode()) {
		if(receiveBuffer->getSize() >= receiveHandler->getLen()) {
			receiveHandler->handle();
		}
	}
	if(transmitHandler) {
		if(transmitBuffer->isEmpty()) {
			transmitHandler->emptyTransmitBuffer();
		}
	}
#else
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
#endif
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

	USART_SendData(usart, transmitBuffer->pop());
}

void Uart::transmit9Bit_isr()
{ 
	if (transmitBuffer->isEmpty() || transmitBuffer->getSize() < 2) {
		stopTransmit();
		return;
	}

	uint8_t setting = transmitBuffer->pop();
	uint16_t data = transmitBuffer->pop();

	if (setting & 0x01) data |= (1 << 8);
	else                data &= ~(1 << 8);

	USART_SendData(usart, data);
}

void Uart::receive_isr(const uint16_t data)
{
	if (mode == Mode_9Bit) {
		receive9Bit_isr(data);
		return;
	}

	receiveBuffer->push(data);

	if(receiveHandler && receiveHandler->isInterruptMode() && receiveBuffer->getSize() >= receiveHandler->getLen()) {
		receiveHandler->handle();
	}

	if (fastForwardingUart) {
		fastForwardingUart->sendAsync(data);
	}
}

void Uart::receive9Bit_isr(const uint16_t data)
{
	uint8_t setting = 1 & (data >> 8);

	receiveBuffer->push(setting);
	receiveBuffer->push(data);

	if(receiveHandler && receiveHandler->isInterruptMode() && receiveBuffer->getSize() >= receiveHandler->getLen()) {
		receiveHandler->handle();
	}

	if (fastForwardingUart) {
		fastForwardingUart->sendAsync(setting);
		fastForwardingUart->sendAsync(data);
	}
}

void Uart::startTransmit()
{
	// Если процесс передачи данных уже запущен, выходим.
	if (isTransmitting) return;

	// Если данных для отправки нет, выходим.
	if (transmitBuffer->isEmpty()) return;

	ATOMIC {
		isTransmitting = true;

		//    (*UCSRB[index]) |= ((1 << TXEN0) | (1 << TXCIE0));
		transmit_isr();
	}
}

void Uart::stopTransmit()
{
	isTransmitting = false;
}

void Uart::enableReceive()
{
	USART_ClearITPendingBit(usart, USART_IT_RXNE);
	USART_ITConfig(usart, USART_IT_RXNE, ENABLE);
}

void Uart::disableReceive()
{
	USART_ITConfig(usart, USART_IT_RXNE, DISABLE);
}

void Uart::clear()
{
	ATOMIC {
		transmitBuffer->clear();
		receiveBuffer->clear();
	}
}

void Uart::disable()
{
	ATOMIC {
		USART_Cmd(usart, DISABLE);
		transmitBuffer->clear();
		receiveBuffer->clear();
		stopTransmit();
	}
}

void Uart::setFastForwardingUart(Uart *uart)
{
	fastForwardingUart = uart;
}

void Uart::clearFastForwardingUart()
{
	fastForwardingUart = NULL;
}

void Uart::disableTransmit()
{
	ATOMIC
	{
		transmitEnable = false;
		stopTransmit();
		transmitBuffer->clear();
	}
}

void Uart::enableTransmit()
{
	ATOMIC
	{
		initGpio();
		transmitEnable = true;
	}
}


void USART_IRQ(USART_TypeDef *uart, enum UARTS uartNum)
{
	// Чистим флаг прерывания
	if (USART_GetITStatus(uart, USART_IT_TC) != RESET) {
		USART_ClearITPendingBit(uart, USART_IT_TC);
		if (pUart[uartNum]) pUart[uartNum]->transmit_isr();
	} else if (USART_GetITStatus(uart, USART_IT_RXNE) != RESET) {
		// Убеждаемся, что прерывание вызвано новыми данными в регистре данных
		// Чистим флаг прерывания
		USART_ClearITPendingBit(uart, USART_IT_RXNE);

		uint16_t data = USART_ReceiveData(uart);
		if (pUart[uartNum]) pUart[uartNum]->receive_isr(data);
	}
}

extern "C" void USART1_IRQHandler(void)
{
	USART_IRQ(USART1, UART_1);
}
extern "C" void USART2_IRQHandler(void)
{
	USART_IRQ(USART2, UART_2);
}
extern "C" void USART3_IRQHandler(void)
{
	USART_IRQ(USART3, UART_3);
}
extern "C" void UART4_IRQHandler(void)
{
	USART_IRQ(UART4, UART_4);
}
extern "C" void UART5_IRQHandler(void)
{
	USART_IRQ(UART5, UART_5);
}
extern "C" void USART6_IRQHandler(void)
{
	USART_IRQ(USART6, UART_6);
}
