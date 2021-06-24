#include <stdio.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx_i2c.h"

#include "I2C.h"
#include "common/logger/include/Logger.h"
#include "common.h"
#include "common/platform/include/platform.h"
#include "common/timer/stm32/include/SystemTimer.h"
#include "common/mdb/MdbProtocol.h"

// DMA Channels and Streams: STM32F4 ref man, page 307: http://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf
//					 I2C1, I2C2, I2C3
static I2C *pI2C[] = { NULL, NULL, NULL };
static DMA_Stream_TypeDef *DMA_TX_STREAM[]	= { DMA1_Stream6, NULL, DMA1_Stream4 };
static DMA_Stream_TypeDef *DMA_RX_STREAM[]	= { DMA1_Stream0, NULL, DMA1_Stream2 };
static uint32_t DMA_TX_CHANNEL[] 			= { DMA_Channel_1, 0, DMA_Channel_3 };
static uint32_t DMA_RX_CHANNEL[]			= { DMA_Channel_1, 0, DMA_Channel_3 };

#define I2C_INTERNAL_OPERATION_TIMEOUT_MS		10
#define I2C_FLAG_MASK							0xffffff
#define I2C_MAX_START_TRANSMISSION_RETRIES		3
#define I2C_DEFAULT_RETRY_PAUSE					2 // Пауза между повторами танзакций по умолчанию

I2C *I2C::get(enI2C index)
{
	if (!pI2C[index])
		return new I2C(index);

	return pI2C[index];
}

I2C::I2C(
	enI2C index,
	uint32_t speed
) :
	index(index),
	busy(false),
	stat(NULL),
	retryPause(I2C_DEFAULT_RETRY_PAUSE),
	eventObserver(NULL),
	mode(Indefined)
{
	pI2C[index] = this;
	i2c = NULL;

	LOG_DEBUG(LOG_I2C, "I2C" << (index + 1) << " creating, speed: " << speed);

	asyncData.isProcess = false;
	i2cDef.I2C_ClockSpeed = speed;

	I2C_Config();
	DMA_Config();
	NVIC_Config();
	setMode(Sync);

	// стартуем модуль I2C
	I2C_Cmd(i2c, ENABLE);
}

void I2C::setObserver(EventObserver *eventObserver)
{
	this->eventObserver = eventObserver;
}

void I2C::setStatStorage(StatStorage *stat)
{
	this->stat = stat;
	this->stat->add(Mdb::DeviceContext::Info_I2C_TransmitTimeout, 0);
	this->stat->add(Mdb::DeviceContext::Info_I2C_TransmitLastEvent, 0);
	this->stat->add(Mdb::DeviceContext::Info_I2C_ReceiveTimeout, 0);
	this->stat->add(Mdb::DeviceContext::Info_I2C_ReceiveLastEvent, 0);
	this->stat->add(Mdb::DeviceContext::Info_I2C_WriteDataTimeout, 0);
	this->stat->add(Mdb::DeviceContext::Info_I2C_WriteDataAckFailure, 0);
	this->stat->add(Mdb::DeviceContext::Info_I2C_ReadDataTimeout, 0);
	this->stat->add(Mdb::DeviceContext::Info_I2C_ReadDataLastEvent, 0);
	this->stat->add(Mdb::DeviceContext::Info_I2C_Busy, 0);
}

void I2C::DMA_Config()
{
	DMA_TxConfig();
	DMA_RxConfig();
}

void I2C::DMA_TxConfig()
{
	DMA_InitTypeDef dmaInitStructure;
	DMA_StructInit(&dmaInitStructure);

	switch ((int) index)
	{

	case I2C_1:
	case I2C_3:
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

		dmaInitStructure.DMA_Channel = DMA_TX_CHANNEL[index];
		dmaInitStructure.DMA_PeripheralBaseAddr = (uint32_t) & i2c->DR;
		dmaInitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
		dmaInitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		dmaInitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		dmaInitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
		dmaInitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
		dmaInitStructure.DMA_Mode = DMA_Mode_Normal;
		dmaInitStructure.DMA_Priority = DMA_Priority_Medium;
		dmaInitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
		dmaInitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
		dmaInitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
		dmaInitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
		DMA_Init(DMA_TX_STREAM[index], &dmaInitStructure);
		break;

	default:
		LOG_ERROR(LOG_I2C, "DMA TxConfig not implemented for I2C" << (index + 1));
		break;
	}
}

void I2C::DMA_RxConfig()
{
	DMA_InitTypeDef dmaInitStructure;
	DMA_StructInit(&dmaInitStructure);

	switch ((int) index)
	{
	case I2C_1:
	case I2C_3:

		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

		dmaInitStructure.DMA_Channel = DMA_RX_CHANNEL[index];
		dmaInitStructure.DMA_PeripheralBaseAddr = (uint32_t) & i2c->DR;
		dmaInitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
		dmaInitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		dmaInitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		dmaInitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
		dmaInitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
		dmaInitStructure.DMA_Mode = DMA_Mode_Normal;
		dmaInitStructure.DMA_Priority = DMA_Priority_Medium;
		dmaInitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
		dmaInitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
		dmaInitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
		dmaInitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
		DMA_Init(DMA_RX_STREAM[index], &dmaInitStructure);
		break;

	default:
		LOG_ERROR(LOG_I2C, "DMA RxConfig not implemented for I2C" << (index + 1));
		break;
	}
}

void I2C::DMA_Config(DMA_Stream_TypeDef* dmaTypeDef, uint8_t *data, int dataSize)
{
	const uint32_t DMA_IT = DMA_IT_TC | /* Прерывание по завершению передачи */
	DMA_IT_TE | /* Прерывание, если произошла ошибка при передаче */
	DMA_IT_DME | /* Прерывание, если произошла ошибка в прямом режиме (direct mode) */
	DMA_IT_FE;

	DMA_ITConfig(dmaTypeDef, DMA_IT, DISABLE);

	DMA_Cmd(dmaTypeDef, DISABLE);

	DMA_MemoryTargetConfig(dmaTypeDef, (uint32_t) data, DMA_Memory_0);
	DMA_SetCurrDataCounter(dmaTypeDef, dataSize);

	DMA_Cmd(dmaTypeDef, ENABLE);

	DMA_ITConfig(dmaTypeDef, DMA_IT, ENABLE);
}

void I2C::NVIC_Config()
{
	NVIC_InitTypeDef NVIC_InitStructure;

	switch ((int) index)
	{

	case I2C_1:
		// TODO: NVIC, I2C1
		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream0_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_I2C1_DMA_RX;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = IRQ_SUB_PRIORITY_I2C1_DMA_RX;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream6_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_I2C1_DMA_TX;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = IRQ_SUB_PRIORITY_I2C1_DMA_TX;
		NVIC_Init(&NVIC_InitStructure);
	break;

	case I2C_3:
		// TODO: NVIC, I2C3
		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream2_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_I2C3_DMA_RX;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = IRQ_SUB_PRIORITY_I2C3_DMA_RX;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream4_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_I2C3_DMA_TX;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = IRQ_SUB_PRIORITY_I2C3_DMA_TX;
		NVIC_Init(&NVIC_InitStructure);
	break;

	default:
		LOG_ERROR(LOG_I2C, "DMA RxConfig not implemented for I2C" << (index + 1));
		break;

	}
}
void I2C::I2C_Config(void)
{
	// настройка I2C
	i2cDef.I2C_Mode = I2C_Mode_I2C;
	i2cDef.I2C_DutyCycle = I2C_DutyCycle_2;

	// адрес любой
	i2cDef.I2C_OwnAddress1 = 0x00;
	i2cDef.I2C_Ack = I2C_Ack_Enable;
	i2cDef.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

	reinit();
}

void I2C::reinit()
{
	GPIO_InitTypeDef gpio;

	switch ((int) index) {
	case I2C_1:
		i2c = I2C1;

		// Включаем тактирование нужных модулей
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); // B8 = I2C1_SCL, B7 = I2C1_SDA

		// I2C использует две ноги микроконтроллера, их тоже нужно настроить
		gpio.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
		gpio.GPIO_Mode = GPIO_Mode_AF;
		gpio.GPIO_Speed = GPIO_Speed_2MHz;
		gpio.GPIO_OType = GPIO_OType_OD;
		gpio.GPIO_PuPd = GPIO_PuPd_UP;


		GPIO_Init(GPIOB, &gpio);

		GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);
		GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_I2C1);
	break;

	case I2C_3:
		i2c = I2C3;

		// Включаем тактирование нужных модулей
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C3, ENABLE);

		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); // A8 = I2C3_SCL
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); // C9 = I2C3_SDA

		// I2C использует две ноги микроконтроллера, их тоже нужно настроить
		gpio.GPIO_Pin = GPIO_Pin_8;
		gpio.GPIO_Mode = GPIO_Mode_AF;
		gpio.GPIO_Speed = GPIO_Speed_2MHz;
		gpio.GPIO_OType = GPIO_OType_OD;
		gpio.GPIO_PuPd = GPIO_PuPd_UP;
		GPIO_Init(GPIOA, &gpio);

		gpio.GPIO_Pin = GPIO_Pin_9;
		GPIO_Init(GPIOC, &gpio);

		GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_I2C3);
		GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_I2C3);
break;

	default:
		LOG_ERROR(LOG_I2C, "DMA RxConfig not implemented for I2C" << (index + 1));
		break;
	}

	ATOMIC
	{
		I2C_DeInit(i2c);
		I2C_Init(i2c, &i2cDef);
	}
	I2C_StretchClockCmd(i2c, DISABLE); // Разрешает устройству останавливать передачу мастера, если тот не готов принимать данные!
//	I2C_StretchClockCmd(i2c, ENABLE); // Запрещает ...
}

void I2C::setMode(enum Mode mode)
{
	if (this->mode == mode)
		return;

	if (mode == Async)
		asyncData.isProcess = false;

	this->mode = mode;
}

bool I2C::startTransmission(const uint8_t deviceAddress, const uint8_t transmissionDirection)
{
	I2C_ClearFlag(i2c, I2C_FLAG_MASK);

	// Генерируем старт
	I2C_GenerateSTOP(i2c, DISABLE);
	I2C_GenerateSTART(i2c, ENABLE);

	// Ждем нужный флаг

	int retries = 3;
	while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT))
	{
		if (!retries--)
		{
			lastEvent = I2C_GetLastEvent(i2c);

			LOG_DEBUG(LOG_I2C, "TIMEOUT, lastEvent: " << Logger::get()->hex32(lastEvent));

			return false;
		}
		SystemTimer::get()->delay_us(100);
	};

	// Посылаем адрес подчиненному
	I2C_Send7bitAddress(i2c, deviceAddress, transmissionDirection);

	uint32_t startTime = SystemTimer::get()->getMs();

	// В зависимости от выбранного направления обмена данными
	if (transmissionDirection == I2C_Direction_Transmitter)
	{
		bool timeout = false;
		while (true)
		{
			lastEvent = I2C_GetLastEvent(i2c);

			if (checkAndClearErrorFlags(lastEvent))
				return false;

			uint32_t awaiting = (I2C_FLAG_BUSY | I2C_FLAG_MSL | I2C_FLAG_TRA | I2C_FLAG_TXE) & I2C_FLAG_MASK;
			if ((lastEvent & awaiting) == awaiting ) return true;

			if (timeout)
			{
				LOG_WARN(LOG_I2C, "Prepare transmit operation timeout! Unknown lastEvent: " << Logger::get()->hex32(lastEvent));
				if(stat)
				{
					stat->get(Mdb::DeviceContext::Info_I2C_TransmitTimeout)->inc();
					stat->get(Mdb::DeviceContext::Info_I2C_TransmitLastEvent)->set(lastEvent);
				}
				return false;
			}

			if (getOperationTimeDiff(startTime) > I2C_INTERNAL_OPERATION_TIMEOUT_MS)
				timeout = true;
		}
	} else if (transmissionDirection == I2C_Direction_Receiver)
	{
		bool timeout = false;
		while (true)
		{
			lastEvent = I2C_GetLastEvent(i2c);

			if (checkAndClearErrorFlags(lastEvent))
				return false;

			uint32_t awaiting = (I2C_FLAG_BUSY | I2C_FLAG_MSL) & I2C_FLAG_MASK;
			if ((lastEvent & awaiting) == awaiting ) return true;

			if (timeout)
			{
				LOG_WARN(LOG_I2C, "Prepare receive operation timeout! Unknown lastEvent: " << Logger::get()->hex(lastEvent));
				if (stat)
				{
					stat->get(Mdb::DeviceContext::Info_I2C_ReceiveTimeout)->inc();
					stat->get(Mdb::DeviceContext::Info_I2C_ReceiveLastEvent)->set(lastEvent);
				}
				return false;
			}

			if (getOperationTimeDiff(startTime) > I2C_INTERNAL_OPERATION_TIMEOUT_MS)
				timeout = true;
		}
	}
	return false;
}

bool I2C::checkAndClearErrorFlags(uint32_t flag)
{
	uint32_t errFlags = I2C_FLAG_MASK & (I2C_FLAG_TIMEOUT | I2C_FLAG_PECERR | I2C_FLAG_OVR | I2C_FLAG_AF | I2C_FLAG_ARLO | I2C_FLAG_BERR);

	if (flag & errFlags)
	{
		if (flag & I2C_FLAG_AF)
		{
			// ACK Failure - чаще всего это нормальное состояние,
			// т.к. используется для проверки возможности записи в микросхему и выдержки таймаута после предыдущей записи.
			LOG_DEBUG(LOG_I2C, "Detected ACK FAILURE flag: " << Logger::get()->hex32(flag));
		}
		else
		{
			LOG_WARN(LOG_I2C, "Detected error event flags: " << Logger::get()->hex32(flag));
		}

		I2C_ClearFlag(i2c, (flag & errFlags));
		return true;
	}

	return false;
}

void I2C::stopTransmission()
{
	I2C_GenerateSTOP(i2c, ENABLE);
}

bool I2C::writeData(uint8_t data)
{
	I2C_SendData(i2c, data);

	uint32_t startTime = SystemTimer::get()->getMs();
	while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
	{
		if (I2C_GetFlagStatus(i2c, I2C_FLAG_AF) == SET)
		{
			LOG_WARN(LOG_I2C, "Detected acknowledge failure flag");
			if(stat) stat->get(Mdb::DeviceContext::Info_I2C_WriteDataAckFailure)->inc();
			return false;
		}

		if (getOperationTimeDiff(startTime) > I2C_INTERNAL_OPERATION_TIMEOUT_MS)
		{
			LOG_WARN(LOG_I2C, "Write data operation timeout!");
			if(stat) stat->get(Mdb::DeviceContext::Info_I2C_WriteDataTimeout)->inc();
			return false;
		}
	}
	return true;
}

bool I2C::syncWriteData(const uint8_t deviceAddress, const uint16_t writeAddress, const uint8_t addressSize, uint8_t *data, const int len, uint32_t timeout)
{
	uint32_t startTime = SystemTimer::get()->getMs();
	bool result;
	do
	{
		result = syncWriteDataImpl(deviceAddress, writeAddress, addressSize, data, len);
		if (!result)
		{
			if (lastEvent != ((I2C_FLAG_BUSY | I2C_FLAG_MSL | I2C_FLAG_AF) & I2C_FLAG_MASK))
			{
				LOG_WARN(LOG_I2C, "I2C" << (index + 1) << " reinit! Last event: " << Logger::get()->hex32(lastEvent));
				SystemTimer::get()->delay_ms(10);

				if (stat)
				{
					stat->get(Mdb::DeviceContext::Info_I2C_WriteDataReinit)->inc();
					stat->get(Mdb::DeviceContext::Info_I2C_ReinitLastEvent)->set(lastEvent);
				}

				reinit();
			}
			else
				SystemTimer::get()->delay_ms(retryPause);
		}

	} while(getOperationTimeDiff(startTime) < timeout && !result);

	return result;
}

bool I2C::syncWriteDataImpl(const uint8_t deviceAddress, const uint16_t writeAddress, const uint8_t addressSize, uint8_t *data, const int len)
{
	if (busy)
	{
		LOG_ERROR(LOG_I2C, "I2C" << (index + 1) << " IS BUSY!");

		if(stat) { stat->get(Mdb::DeviceContext::Info_I2C_Busy)->inc(); }
		return false;
	}

	busy = true;

	setMode(Sync);

	I2C_AcknowledgeConfig(i2c, DISABLE);

	if (!startTransmission(deviceAddress, I2C_Direction_Transmitter))
	{
		stopTransmission();
		busy = false;
		return false;
	}

	if (len)
	{
		if (addressSize == 1)
		{
			if (!writeData(LOBYTE(writeAddress)))
			{
				busy = false;
				return false;
			}
		}
		else if (addressSize == 2)
		{
			if (!writeData(HIBYTE(writeAddress)) || !writeData(LOBYTE(writeAddress)))
			{
				busy = false;
				return false;
			}
		}

		for (int i = 0; i < len; i++)
			if (!writeData(data[i]))
			{
				busy = false;
				return false;
			}
	}

	stopTransmission();
	busy = false;
	return true;
}

bool I2C::syncReadData(const uint8_t deviceAddress, const uint16_t readAddress, const uint8_t addressSize, uint8_t *data, const int len, uint32_t timeout)
{
	uint32_t startTime = SystemTimer::get()->getMs();
	bool result;
	do
	{
		result = syncReadDataImpl(deviceAddress, readAddress, addressSize, data, len);
		if (!result)
		{
			if (lastEvent != ((I2C_FLAG_BUSY | I2C_FLAG_MSL | I2C_FLAG_AF) & I2C_FLAG_MASK))
			{
				LOG_WARN(LOG_I2C, "I2C" << (index + 1) << " reinit! Last event: " << Logger::get()->hex32(lastEvent));
				SystemTimer::get()->delay_ms(10);

				if (stat)
				{
					stat->get(Mdb::DeviceContext::Info_I2C_ReadDataReinit)->inc();
					stat->get(Mdb::DeviceContext::Info_I2C_ReinitLastEvent)->set(lastEvent);
				}

				reinit();
			}
			else
				SystemTimer::get()->delay_ms(retryPause);
		}
	} while (getOperationTimeDiff(startTime) < timeout && !result);

	return result;
}

bool I2C::syncReadDataImpl(const uint8_t deviceAddress, const uint16_t readAddress, const uint8_t addressSize, uint8_t *data, const int len)
{
	if (busy)
	{
		LOG_ERROR(LOG_I2C, "I2C" << (index + 1) << " IS BUSY!");

		if(stat) { stat->get(Mdb::DeviceContext::Info_I2C_Busy)->inc(); }
		return false;
	}

	busy = true;

	setMode(Sync);

	I2C_AcknowledgeConfig(i2c, DISABLE);

	if (addressSize > 0 && !startTransmission(deviceAddress, I2C_Direction_Transmitter))
	{
		busy = false;
		return false;
	}

	if (addressSize == 1)
	{
		if (!writeData(LOBYTE(readAddress)))
		{
			busy = false;
			return false;
		}
	}
	else if (addressSize == 2)
	{
		if (!writeData(HIBYTE(readAddress)) || !writeData(LOBYTE(readAddress)))
		{
			busy = false;
			return false;
		}
	}

	if (len != 1)
		I2C_AcknowledgeConfig(i2c, ENABLE);

	if (!startTransmission(deviceAddress, I2C_Direction_Receiver))
	{
		busy = false;
		return false;
	}

	int numBytesToRead = len;
	int index = 0;


	while (numBytesToRead)
	{
		uint32_t startTime = SystemTimer::get()->getMs();
		bool timeout = false;
		while (true)
		{
			uint32_t lastEvent = I2C_GetLastEvent(i2c);

			if (checkAndClearErrorFlags(lastEvent))
			{
				busy = false;
				return false;
			}

			if (lastEvent == I2C_EVENT_MASTER_BYTE_RECEIVED)
				break;

			if (lastEvent == (I2C_EVENT_MASTER_BYTE_RECEIVED | (I2C_FLAG_BTF & I2C_FLAG_MASK)))
				break;

			if (timeout)
			{
				LOG_WARN(LOG_I2C, "Read data operation timeout! Unknown lastEvent: " << Logger::get()->hex32(lastEvent));
				if (stat)
				{
					stat->get(Mdb::DeviceContext::Info_I2C_ReadDataTimeout)->inc();
					stat->get(Mdb::DeviceContext::Info_I2C_ReadDataLastEvent)->set(lastEvent);
				}
				busy = false;
				return false;
			}

			if (getOperationTimeDiff(startTime) > I2C_INTERNAL_OPERATION_TIMEOUT_MS)
				timeout = true;
		}

		if (numBytesToRead == 2)
			I2C_AcknowledgeConfig(i2c, DISABLE);

		data[index++] = I2C_ReceiveData(i2c);
		numBytesToRead--;
	}

	I2C_AcknowledgeConfig(i2c, ENABLE);
	stopTransmission();

	busy = false;
	return true;
}

bool I2C::asyncWriteData(const uint8_t deviceAddress, const uint16_t writeAddress, const uint8_t addressSize,  uint8_t *data, const int len, const uint16_t eventType)
{
	if (asyncData.isProcess)
		return false;
	setMode(Async);

	DMA_Config(DMA_TX_STREAM[index], data, len);

	/* Enable DMA NACK automatic generation */
	I2C_DMALastTransferCmd(i2c, ENABLE);

	if (!startTransmission(deviceAddress, I2C_Direction_Transmitter))
		return false;

	// Адрес для записи
	if (addressSize == 1)
	{
		writeData(LOBYTE(writeAddress));
	}
	else
	{
		writeData(HIBYTE(writeAddress));
		writeData(LOBYTE(writeAddress));
	}

	asyncData.isProcess = true;
	asyncData.error = false;
	asyncData.eventType = eventType;

	/* Start DMA to receive data from I2C */
	DMA_Cmd(DMA_TX_STREAM[index], ENABLE);
	I2C_DMACmd(i2c, ENABLE);

	// По окончании приема данных попадаем в прерывание DMA TX

	return true;
}

bool I2C::asyncReadData(const uint8_t deviceAddress, const uint16_t readAddress, const uint8_t addressSize, uint8_t *data, const int len, const uint16_t eventType)
{
	if (asyncData.isProcess)
		return false;

	setMode(Async);

	DMA_Config(DMA_RX_STREAM[index], data, len);

	/* Enable DMA NACK automatic generation */
	I2C_DMALastTransferCmd(i2c, ENABLE);

	if (!startTransmission(deviceAddress, I2C_Direction_Transmitter))
		return false;

	// Адрес чтения
	if (addressSize == 1)
	{
		writeData(LOBYTE(readAddress));
	}
	else
	{
		writeData(HIBYTE(readAddress));
		writeData(LOBYTE(readAddress));
	}

	I2C_AcknowledgeConfig(i2c, ENABLE);
	if (!startTransmission(deviceAddress, I2C_Direction_Receiver))
		return false;

	/* Start DMA to receive data from I2C */
	DMA_Cmd(DMA_RX_STREAM[index], ENABLE);
	I2C_DMACmd(i2c, ENABLE);

	// По окончании приема данных попадаем в прерывание DMA RX

	asyncData.isProcess = true;
	asyncData.error = false;
	asyncData.eventType = eventType;
	return true;
}

enum I2C::Mode I2C::getMode()
{
	return mode;
}

uint32_t I2C::getOperationTimeDiff(uint32_t lastTimeMs)
{
	return SystemTimer::get()->getCurrentAndLastTimeDiff(lastTimeMs);
}
void I2C::setRetryPause(int mls)
{
	retryPause = mls;
}

// =============================================================================================
extern "C" void DMA_Stream_IRQHandler(enum enI2C index, DMA_Stream_TypeDef *dmaStream[], uint32_t dmaFlag_TCIF)
{
	I2C_TypeDef *i2c = I2C::get(index)->i2c;
	DMA_Stream_TypeDef *stream = dmaStream[index];

	if (DMA_GetFlagStatus(stream, dmaFlag_TCIF) == SET)
	{
		DMA_ClearFlag(stream, dmaFlag_TCIF);
	} else
	{
		LOG_ERROR(LOG_I2C, "Error!");
		I2C::get(index)->asyncData.error = true;
	}

	I2C_DMACmd(i2c, DISABLE);
	I2C_GenerateSTOP(i2c, ENABLE);
	DMA_Cmd(stream, DISABLE);

	I2C::get(index)->asyncData.isProcess = false;

	if (I2C::get(index)->eventObserver != NULL)
	{
		Event event(I2C::get(index)->asyncData.eventType,
				(uint8_t) I2C::get(index)->asyncData.error);
		I2C::get(index)->eventObserver->proc(&event);
	}
}
// ====================================== Прерывания I2C3 ======================================
extern "C" void DMA1_Stream2_IRQHandler()
{
	LOG_TRACE(LOG_I2C, "DMA1_Stream2, RX");

	DMA_Stream_IRQHandler(I2C_3, DMA_RX_STREAM, DMA_FLAG_TCIF2);
}

extern "C" void DMA1_Stream4_IRQHandler()
{
	LOG_TRACE(LOG_I2C, "DMA1_Stream4, TX");

	DMA_Stream_IRQHandler(I2C_3, DMA_TX_STREAM, DMA_FLAG_TCIF4);
}
// ====================================== Прерывания I2C1 ======================================
extern "C" void DMA1_Stream0_IRQHandler()
{
	LOG_TRACE(LOG_I2C, "DMA1_Stream0, RX");

	DMA_Stream_IRQHandler(I2C_1, DMA_RX_STREAM, DMA_FLAG_TCIF0);
}

extern "C" void DMA1_Stream6_IRQHandler()
{
	LOG_TRACE(LOG_I2C, "DMA1_Stream6, TX");

	DMA_Stream_IRQHandler(I2C_1, DMA_TX_STREAM, DMA_FLAG_TCIF6);
}

