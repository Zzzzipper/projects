#include <stdio.h>

#include "stm32f4xx_conf.h"
#include "common/logger/include/Logger.h"
#include "common/utils/stm32/include/Led.h"
#include "Adc.h"


static Adc *INSTANCE_ADC = NULL;

#define ADCx                     ADC1
#define ADC_CHANNEL              ADC_Channel_1
#define ADCx_CLK                 RCC_APB2Periph_ADC1
#define DMA_CHANNELx             DMA_Channel_0 // = ADC1
#define DMA_STREAMx              DMA2_Stream0
#define ADCx_DR_ADDRESS          (uint32_t)&ADCx->DR

// Делитель напряжения, R1, R2, для каждого канала
const uint16_t RES_DIVIDERS[ADC_REG_CHANNELS][2] = {{1000, 1000}, {10000, 620}, {2000, 1000} , {1000, 1000}, {0, 0}, {0, 0}, {0, 0}, {1, 1}};

Adc *Adc::get()
{
	if (!INSTANCE_ADC) INSTANCE_ADC = new Adc();
	return INSTANCE_ADC;
}

Adc::Adc(): index(0)
{
	const int size = ADC_PROCESS_AVERAGE_COUNT;

	avg = new FastAverage<uint16_t>* [ADC_REG_CHANNELS];

	for (int i = 0; i < ADC_REG_CHANNELS; i++)
		avg[i] = new FastAverage<uint16_t>(size);

	TIM8_Config();
	DMA_Config();
	NVIC_Configuration();
	ADC_Config();

	ADC_SoftwareStartConv(ADCx);
}

void Adc::DMA_Config()
{
	  DMA_InitTypeDef       DMA_InitStructure;

	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

	  /* DMA2 Stream0 channel2 configuration **************************************/
	  DMA_InitStructure.DMA_Channel = DMA_CHANNELx;
	  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADCx_DR_ADDRESS;
	  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&regularValues;
	  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	  DMA_InitStructure.DMA_BufferSize = ADC_REG_CHANNELS;
	  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	  DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_HalfWord;
	  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable; // Dis
	  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	  DMA_Init(DMA_STREAMx, &DMA_InitStructure);

	  DMA_Cmd(DMA_STREAMx, ENABLE);

	  DMA_ITConfig(DMA_STREAMx,
			  DMA_IT_TC		// Прерывание по завершению передачи
//			  | DMA_IT_HT	// Прерывание по завершению передачи половины буфера
//			  | DMA_IT_TE	// Прерывание, если произошла ошибка при передаче
//			  | DMA_IT_DME 	// Прерывание, если произошла ошибка в прямом режиме (direct mode)
//			  | DMA_IT_FE 	// Прерывание, если произошла ошибка FIFO буфера
			  , ENABLE);
}


void Adc::ADC_Config(void)
{
  ADC_InitTypeDef       ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  GPIO_InitTypeDef gpio;

  /* Enable ADCx, DMA and GPIO clocks ****************************************/
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(ADCx_CLK, ENABLE);


  /* Configure ADC Channel pin as analog input ******************************/
  GPIO_StructInit(&gpio);
  gpio.GPIO_Mode = GPIO_Mode_AN;
  gpio.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
  gpio.GPIO_Speed = GPIO_High_Speed;
  GPIO_Init(GPIOA, &gpio);

  gpio.GPIO_Pin = GPIO_Pin_1;
  GPIO_Init(GPIOB, &gpio);

  /* ADC Common Init **********************************************************/
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div8;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;        // None
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_10Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);

  /* ADC Init ****************************************************************/
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T8_TRGO; //ADC_ExternalTrigConv_T1_CC1;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = ADC_REG_CHANNELS;
  ADC_Init(ADCx, &ADC_InitStructure);

  /* ADC regular channel configuration **************************************/
  uint8_t cycles = ADC_SampleTime_112Cycles;

  ADC_RegularChannelConfig(ADCx, ADC_Channel_1, VCC_3+1, cycles);
  ADC_RegularChannelConfig(ADCx, ADC_Channel_4, VCC_24+1, cycles);
  ADC_RegularChannelConfig(ADCx, ADC_Channel_5, VCC_5+1, cycles);
  ADC_RegularChannelConfig(ADCx, ADC_Channel_6, VCC_BAT1+1, cycles);
  ADC_RegularChannelConfig(ADCx, ADC_Channel_9, VCC_EXT_TEMP+1, cycles);
  ADC_RegularChannelConfig(ADCx, ADC_Channel_Vrefint, VCC_REF+1, cycles);
  ADC_RegularChannelConfig(ADCx, ADC_Channel_TempSensor, TEMP_SENSOR+1, cycles);
  ADC_RegularChannelConfig(ADCx, ADC_Channel_Vbat, VCC_BAT2+1, cycles);

  ADC_TempSensorVrefintCmd(ENABLE);
  ADC_VBATCmd(ENABLE);


 /* Enable DMA request after last transfer (Single-ADC mode) */
  ADC_DMARequestAfterLastTransferCmd(ADCx, ENABLE);

  /* Enable ADC DMA */
  ADC_DMACmd(ADCx, ENABLE);

  /* Enable ADC */
  ADC_Cmd(ADCx, ENABLE);
}

void Adc::TIM8_Config()
{
	// Настраиваем таймер 8 на прерывание 4 раза в сек
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
	TIM_TimeBaseInitTypeDef timerStruct;

	timerStruct.TIM_Period = 5000;
	timerStruct.TIM_Prescaler = 8400;
	timerStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	timerStruct.TIM_CounterMode = TIM_CounterMode_Up;
	timerStruct.TIM_RepetitionCounter = 0x0000;

	TIM_TimeBaseInit(TIM8, &timerStruct);

	TIM_SelectOutputTrigger(TIM8, TIM_TRGOSource_Update);
	TIM_Cmd(TIM8, ENABLE);
}

void Adc::NVIC_Configuration(void)
{
	NVIC_InitTypeDef  NVIC_InitStructure;

	// TODO: NVIC, ADC
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_ADC;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = IRQ_SUB_PRIORITY_ADC;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void Adc::placeRegularValues()
{
	for (int i = 0; i < ADC_REG_CHANNELS; i++)
			avg[i]->add(regularValues[i]);

	LOG_TRACE(LOG_ADC, "ADC, VCC_3: " << read(Adc::VCC_3) << ", " <<
			"VCC_5: " << read(Adc::VCC_5) << ", " <<
			"VCC_24: " << read(Adc::VCC_24) << ", " <<
			"VCC_BAT1: " << read(Adc::VCC_BAT1) << ", " <<
			"TEMP_SENSOR: " << read(Adc::TEMP_SENSOR) << ", " <<
			"VCC_BAT2: " << read(Adc::VCC_BAT2));
}

uint32_t Adc::read(Rank rank)
{
	float result;
	float ref = __LL_ADC_CALC_VREFANALOG_VOLTAGE(avg[VCC_REF]->getAverage(), LL_ADC_RESOLUTION_12B);

	if (rank == TEMP_SENSOR)
	{
		result = __LL_ADC_CALC_TEMPERATURE(ref, avg[rank]->getAverage(), LL_ADC_RESOLUTION_12B);
	} else
	{
		result = __LL_ADC_CALC_DATA_TO_VOLTAGE(ref, avg[rank]->getAverage(), LL_ADC_RESOLUTION_12B);
	}

	// Uin = (R1 + R2) * value / R2
	float r1 = RES_DIVIDERS[rank][0];
	float r2 = RES_DIVIDERS[rank][1];

	if (r1 > 0 && r2 > 0)
		result = (r1 + r2) * result / r2;

	return result;
}

uint32_t Adc::getCpuTemp()
{
	return read(TEMP_SENSOR);
}

uint32_t Adc::getInputVoltage()
{
	return read(VCC_24);
}

extern "C" void DMA2_Stream0_IRQHandler(void)
{
	/* Test on DMA Stream Transfer Complete interrupt */
	if(DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0))
	{
    /* Clear DMA Stream Transfer Complete interrupt pending bit */
		DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);

		if (INSTANCE_ADC)
			INSTANCE_ADC->placeRegularValues();
	} else
	{
		LOG_ERROR(LOG_ADC, "Unknown status");
	}
}
