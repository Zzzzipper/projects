#ifndef __UART2_H
#define __UART2_H

#include <stdint.h>

#include "stm32f4xx_conf.h"

#include "common.h"
#include "defines.h"
#include "utils/include/StringBuilder.h"
#include "utils/include/Fifo.h"
#include "uart/include/interface.h"


#define UART_MAX_SPEED			256000
#define UART_DEFAULT_SPEED		9600
#define UART_DEFAULT_FIFO_SIZE  32
#define UART_DEFAULT_OTYPE		GPIO_OType_PP

class StringBuilder;

enum UARTS {
	UART_1,
	UART_2,
	UART_3,
	UART_4,
	UART_5,
	UART_6
};


class Uart : public AbstractUart {
public:
  enum Parity {
    Parity_Unknown = -1,
    Parity_None = USART_Parity_No,
    Parity_Odd = USART_Parity_Odd,
    Parity_Even = USART_Parity_Even
  };

  enum Mode {
    Mode_Normal,
    Mode_9Bit
  };

  static Uart *get(UARTS uart);
  Uart(UARTS index, uint32_t speed, Parity parity, uint8_t stop_bit);
  Uart *setup(uint32_t speed, Parity parity, uint8_t stop_bit);
  Uart *setup(uint32_t speed, Parity parity, uint8_t stop_bit, Mode mode);
  //! Настройка параметров УАРТ.
  //! \param speed - скорость, бит/сек
  //! \param parity - четность.
  //! \param stop_bit - кол-во стоповых бит.
  //! \param output_type - тип порта, Push-Pull(GPIO_OType_PP) или Open-Drain (GPIO_OType_OD)
  //! \param mode - режим порта
  Uart *setup(uint32_t speed, Parity parity, uint8_t stop_bit, Mode mode, GPIOOType_TypeDef output_type);
  void setTransmitBufferSize(uint32_t bufSize);
  void setTransmitBuffer(uint8_t *buf, uint32_t bufSize);
  void setReceiveBufferSize(uint32_t bufSize);
  void setReceiveBuffer(uint8_t *buf, uint32_t bufSize);

  virtual void setReceiveHandler(UartReceiveHandler *handler) override;
  virtual void setTransmitHandler(UartTransmitHandler *handler) override;
  virtual void send(uint8_t b) override;
  virtual void sendAsync(uint8_t b) override;
  virtual BYTE receive() override;
  virtual bool isEmptyReceiveBuffer() override;
  virtual bool isFullTransmitBuffer() override;
  virtual void execute() override;

  bool isEmptyTransmitBuffer();

  void clear();
  void setFastForwardingUart(Uart *uart);
  void clearFastForwardingUart();

  // Отключаем передачу.
  void disableTransmit();
  void enableTransmit();

  GPIO_TypeDef *getTxPort();
  uint32_t getTxPinSource();
  GPIOOType_TypeDef getTxOutputType();

  void transmit_isr();
  void receive_isr(const uint16_t data);

private:
  USART_TypeDef* usart;
  UartReceiveHandler *receiveHandler;
  UartTransmitHandler *transmitHandler;
  Mode mode;
  Uart* fastForwardingUart;	// Уарт, на который будут пробрасываться все принятые данные
  GPIOOType_TypeDef output_type;

  inline void enableReceive();
  inline void disableReceive();

  void transmit9Bit_isr();
  void receive9Bit_isr(const uint16_t data);

  // Отключаем UART от периферии. Это позволит управлять ногами, занятыми UART. Для инициализации приема неоходимо вызвать enableReceive();
  void disable();

protected:
  //! Номер UART
  UARTS index;
  Fifo<uint8_t> *receiveBuffer;
  Fifo<uint8_t> *transmitBuffer;
  volatile bool isTransmitting;

  GPIO_InitTypeDef  gpio_InitStructure;
  USART_InitTypeDef usart_InitStructure;
  bool transmitEnable;

  void startTransmit();
  void stopTransmit();
  inline bool isTransmitEmpty();
  void setParity(const Parity parity);
  void initGpio();


};

#endif
