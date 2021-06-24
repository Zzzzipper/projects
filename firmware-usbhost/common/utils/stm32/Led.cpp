
#include "config.h"
#include "include/Led.h"
#include "common/timer/stm32/include/SystemTimer.h"

static Led *INSTANCE = NULL;

typedef GPIO_TypeDef *	P_GPIO;
typedef USART_TypeDef*	P_USART_TypeDef;

const P_GPIO 	LED_PORT[]	 = {GPIOE,	   GPIOE,      GPIOD,		GPIOD};
const uint32_t	LED_R_PIN[] = {GPIO_Pin_2, GPIO_Pin_0, GPIO_Pin_5,	GPIO_Pin_3};
const uint32_t	LED_G_PIN[] = {GPIO_Pin_3, GPIO_Pin_1, GPIO_Pin_6,	GPIO_Pin_4};


Led *Led::get()
{
	if (INSTANCE == NULL) INSTANCE = new Led();
	return INSTANCE;
}

Led::Led()
{
#if (HW_VERSION == HW_2_0_0)
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_OType = GPIO_OType_OD;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_Init(GPIOB, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOC, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_11 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_Init(GPIOE	, &gpio);
#elif (HW_VERSION >= HW_3_0_0 && HW_VERSION < HW_3_1_0)

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_OType = GPIO_OType_OD;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOE, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_Init(GPIOD, &gpio);

#elif (HW_VERSION >= HW_3_1_0 && HW_VERSION < HW_3_3_0)

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM11, ENABLE);


	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_OType = GPIO_OType_OD;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOE, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_Init(GPIOD, &gpio);

	// PWM
	gpio.GPIO_Pin = GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_Init(GPIOB, &gpio);

	// Переконфигурируем выход для работы с таймером. Теперь сигнал формируемый таймером будет идти прямо на вывод контроллера.
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_TIM11);

	// Конфигурация таймера
	TIM_TimeBaseInitTypeDef TIM_BaseConfig;
	// Конфигурация выхода таймера
	TIM_OCInitTypeDef TIM_OCConfig;

    // Запускаем таймер на тактовой частоте в 40 kHz
    TIM_BaseConfig.TIM_Prescaler = (uint16_t) (SystemCoreClock / 40000) - 1;
    // Период - 255 тактов => 40000/255 = 156 Hz
    TIM_BaseConfig.TIM_Period = 255;
    TIM_BaseConfig.TIM_ClockDivision = TIM_CKD_DIV1;
    // Отсчет от нуля до TIM_Period
    TIM_BaseConfig.TIM_CounterMode = TIM_CounterMode_Up;

    // Инициализируем таймер №11
    TIM_TimeBaseInit(TIM11, &TIM_BaseConfig);

    // Конфигурируем выход таймера, режим - PWM1
    TIM_OCConfig.TIM_OCMode = TIM_OCMode_PWM1;
    // Собственно - выход включен
    TIM_OCConfig.TIM_OutputState = TIM_OutputState_Enable;
    // Пульс
    TIM_OCConfig.TIM_Pulse = 255/2;
    // Полярность => пульс - это GND
    TIM_OCConfig.TIM_OCPolarity = TIM_OCPolarity_Low;

    // Инициализируем первый выход таймера №11
    TIM_OC1Init(TIM11, &TIM_OCConfig);

    TIM_OC1PreloadConfig(TIM11, TIM_OCPreload_Enable);

    // Включаем таймер
    TIM_Cmd(TIM11, ENABLE);
#elif (HW_VERSION >= HW_3_3_0)

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOE, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_Init(GPIOD, &gpio);


#else
#error "HW_VERSION must be defined in project settings"
#endif
}

Led *Led::setLed(int index, BYTE r, BYTE g)
{
	if(g == r)
	{
		GPIO_ResetBits(LED_PORT[index], LED_R_PIN[index]);
		GPIO_ResetBits(LED_PORT[index], LED_G_PIN[index]);
	}
	else if(g)
	{
		GPIO_SetBits(LED_PORT[index], LED_G_PIN[index]);
		GPIO_ResetBits(LED_PORT[index], LED_R_PIN[index]);
	}
	else if(r)
	{
		GPIO_SetBits(LED_PORT[index], LED_R_PIN[index]);
		GPIO_ResetBits(LED_PORT[index], LED_G_PIN[index]);
	}

	return this;
}

Led *Led::setLed1(BYTE r, BYTE g)
{
#if (HW_VERSION == HW_2_0_0)
	if (g)	GPIO_ResetBits(GPIOE, GPIO_Pin_13);
	else		GPIO_SetBits(GPIOE, GPIO_Pin_13);

	if (r)	GPIO_ResetBits(GPIOE, GPIO_Pin_14);
	else		GPIO_SetBits(GPIOE, GPIO_Pin_14);
#elif (HW_VERSION >= HW_3_0_0 && HW_VERSION < HW_3_3_0)
	if (g)	GPIO_ResetBits(GPIOE, GPIO_Pin_3);
	else		GPIO_SetBits(GPIOE, GPIO_Pin_3);

	if (r)	GPIO_ResetBits(GPIOE, GPIO_Pin_2);
	else		GPIO_SetBits(GPIOE, GPIO_Pin_2);
#elif (HW_VERSION >= HW_3_3_0)

	setLed(0, r, g);

#else
#error "HW_VERSION must be defined in project settings"
#endif

	return this;
}

Led *Led::setLed2(BYTE r, BYTE g)
{
#if (HW_VERSION == HW_2_0_0)
	if (g)	GPIO_ResetBits(GPIOB, GPIO_Pin_0);
	else		GPIO_SetBits(GPIOB, GPIO_Pin_0);

	if (r)	GPIO_ResetBits(GPIOB, GPIO_Pin_1);
	else		GPIO_SetBits(GPIOB, GPIO_Pin_1);
#elif (HW_VERSION >= HW_3_0_0 && HW_VERSION < HW_3_3_0)
	if (g)	GPIO_ResetBits(GPIOE, GPIO_Pin_1);
	else		GPIO_SetBits(GPIOE, GPIO_Pin_1);

	if (r)	GPIO_ResetBits(GPIOE, GPIO_Pin_0);
	else		GPIO_SetBits(GPIOE, GPIO_Pin_0);
#elif (HW_VERSION >= HW_3_3_0)

	setLed(1, r, g);

#else
#error "HW_VERSION must be defined in project settings"
#endif

	return this;
}

Led *Led::setLed3(BYTE r, BYTE g)
{
#if (HW_VERSION == HW_2_0_0)
	if (g)	GPIO_ResetBits(GPIOC, GPIO_Pin_6);
	else		GPIO_SetBits(GPIOC, GPIO_Pin_6);

	if (r)	GPIO_ResetBits(GPIOC, GPIO_Pin_7);
	else		GPIO_SetBits(GPIOC, GPIO_Pin_7);
#elif (HW_VERSION >= HW_3_0_0 && HW_VERSION < HW_3_3_0)
	if (g)	GPIO_ResetBits(GPIOD, GPIO_Pin_6);
	else		GPIO_SetBits(GPIOD, GPIO_Pin_6);

	if (r)	GPIO_ResetBits(GPIOD, GPIO_Pin_5);
	else		GPIO_SetBits(GPIOD, GPIO_Pin_5);
#elif (HW_VERSION >= HW_3_3_0)

	setLed(2, r, g);

#else
#error "HW_VERSION must be defined in project settings"
#endif

	return this;
}

Led *Led::setLed4(BYTE r, BYTE g)
{
#if (HW_VERSION == HW_2_0_0)
	if (g)	GPIO_ResetBits(GPIOE, GPIO_Pin_9);
	else		GPIO_SetBits(GPIOE, GPIO_Pin_9);

	if (r)	GPIO_ResetBits(GPIOE, GPIO_Pin_11);
	else		GPIO_SetBits(GPIOE, GPIO_Pin_11);
#elif (HW_VERSION >= HW_3_0_0 && HW_VERSION < HW_3_3_0)
	if (g)	GPIO_ResetBits(GPIOD, GPIO_Pin_4);
	else		GPIO_SetBits(GPIOD, GPIO_Pin_4);

	if (r)	GPIO_ResetBits(GPIOD, GPIO_Pin_3);
	else		GPIO_SetBits(GPIOD, GPIO_Pin_3);
#elif (HW_VERSION >= HW_3_3_0)

	setLed(3, r, g);

#else
#error "HW_VERSION must be defined in project settings"
#endif

	return this;
}


Led *Led::allFlash(int cnt, int delay)
{
	while(cnt--)
	{
		setLed1(255, 0);
		setLed2(255, 0);
		setLed3(255, 0);
		setLed4(255, 0);

		SystemTimer::get()->delay_ms(delay);

		setLed1(0, 255);
		setLed2(0, 255);
		setLed3(0, 255);
		setLed4(0, 255);

		SystemTimer::get()->delay_ms(delay);

		setLed1(0, 0);
		setLed2(0, 0);
		setLed3(0, 0);
		setLed4(0, 0);

		SystemTimer::get()->delay_ms(delay);
	}
	return this;
}


Led *Led::redFlash(int cnt, int delay)
{
	while(cnt--)
	{
		setLed1(255, 0);
		setLed2(255, 0);
		setLed3(255, 0);
		setLed4(255, 0);

		SystemTimer::get()->delay_ms(delay);

		setLed1(0, 0);
		setLed2(0, 0);
		setLed3(0, 0);
		setLed4(0, 0);

		SystemTimer::get()->delay_ms(delay);
	}
	return this;
}

Led *Led::greenFlash(int cnt, int delay)
{
	while(cnt--)
	{
		setLed1(0, 255);
		setLed2(0, 255);
		setLed3(0, 255);
		setLed4(0, 255);

		SystemTimer::get()->delay_ms(delay);

		setLed1(0, 0);
		setLed2(0, 0);
		setLed3(0, 0);
		setLed4(0, 0);

		SystemTimer::get()->delay_ms(delay);
	}
	return this;
}

Led *Led::setPower(uint8_t value)
{
#if (HW_VERSION >= HW_3_1_0 && HW_VERSION < HW_3_3_0)
	TIM11->CCR1 = value;
#endif
	return this;
}


