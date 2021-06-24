#include "buttons.h"
#include "defines.h"

#include "logger/include/Logger.h"

class Button1 : public Button {
public:
	Button1() : Button(Buttons::Event_Button1) {}
	bool checkPressed() {
#if (HW_VERSION == HW_2_0_0)
		return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) == (uint8_t)Bit_RESET;
#elif (HW_VERSION >= HW_3_0_0)
		return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_6) == (uint8_t)Bit_RESET;
#else
 #error "HW_VERSION must be defined in project settings"
#endif
	}
};

class Button2 : public Button {
public:
	Button2() : Button(Buttons::Event_Button2) {}
	bool checkPressed() {
#if (HW_VERSION == HW_2_0_0)
		return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8) == (uint8_t)Bit_RESET;
#elif (HW_VERSION >= HW_3_0_0)
		return GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == (uint8_t)Bit_RESET;
#else
 #error "HW_VERSION must be defined in project settings"
#endif
	}
};

class Button3 : public Button {
public:
	Button3() : Button(Buttons::Event_Button3) {}
	bool checkPressed() {
#if (HW_VERSION == HW_2_0_0)
		return GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9) == (uint8_t)Bit_RESET;
#elif (HW_VERSION >= HW_3_0_0)
		return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_8) == (uint8_t)Bit_RESET;
#else
 #error "HW_VERSION must be defined in project settings"
#endif
	}
};

class Button4 : public Button {
public:
	Button4() : Button(Buttons::Event_Button4) {}
	bool checkPressed() {

#if (HW_VERSION == HW_2_0_0)
		return false;
#elif (HW_VERSION >= HW_3_0_0)
		return GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_12) == (uint8_t)Bit_RESET;
#else
 #error "HW_VERSION must be defined in project settings"
#endif
	}
};

class Button5 : public Button {
public:
	Button5() : Button(Buttons::Event_Button5) {}
	bool checkPressed() {
#if (HW_VERSION == HW_2_0_0)
		return false;
#elif (HW_VERSION >= HW_3_0_0)
		return GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_10) == (uint8_t)Bit_RESET;
#else
 #error "HW_VERSION must be defined in project settings"
#endif
	}
};

class Button6 : public Button {
public:
	Button6() : Button(Buttons::Event_Button6) {}
	bool checkPressed() {
#if (HW_VERSION == HW_2_0_0) || (HW_VERSION >= HW_3_0_2)
		return false;
#elif (HW_VERSION == HW_3_0_0)
		return GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11) == (uint8_t)Bit_RESET;
#else
 #error "HW_VERSION must be defined in project settings"
#endif
	}
};

Buttons::Buttons() : ButtonEngine(6) {
	buttons->add(new Button1());
	buttons->add(new Button2());
	buttons->add(new Button3());
	buttons->add(new Button4());
	buttons->add(new Button5());
#if (HW_VERSION < HW_3_0_2)
	buttons->add(new Button6());
#endif
}

Buttons::~Buttons() {
	shutdownButtons();
}

void Buttons::initButtons() {
#if (HW_VERSION == HW_2_0_0)
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	gpio.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOB, &gpio);
#elif (HW_VERSION == HW_3_0_0)
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_8;
	GPIO_Init(GPIOE, &gpio);
	gpio.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOC, &gpio);
	gpio.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_Init(GPIOD, &gpio);
#elif (HW_VERSION >= HW_3_0_2)
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_8;
	GPIO_Init(GPIOE, &gpio);
	gpio.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOC, &gpio);
	gpio.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_10;
	GPIO_Init(GPIOD, &gpio);
#else
 #error "HW_VERSION must be defined in project settings"
#endif
}

void Buttons::shutdownButtons() {}

