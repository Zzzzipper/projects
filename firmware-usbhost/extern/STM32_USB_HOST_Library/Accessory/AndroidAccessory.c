#include "AndroidAccessory.h"
#include "usbh_vendor_ibox.h"

void HAL_Delay(uint32_t delay);

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);
uint32_t USBH_ALL_TransmitRequest(uint8_t *writeData, const uint32_t maxLen);
void USBH_Ibox_ReceiveCallback(uint8_t *data, uint32_t len);
void USBH_Ibox_TransmitCallback(USBH_HandleTypeDef *phost);


StartUsbCallback startUsbCallback = NULL;

AA androidAccessory;

uint8_t protocolVersion[2];
uint16_t vendorID = -1;
uint16_t productID = -1;

USBH_HandleTypeDef hUsbHostFS;
ApplicationTypeDef Appli_state = APPLICATION_IDLE;

void registerStartUsbCallback(StartUsbCallback func_) {
	startUsbCallback = func_;
}

void AndroidAccessory(const char *manufacturer, const char *model,
		const char *description, const char *version, const char *uri,
		const char *serial, uint8_t isAudio) {
	USBH_UsrLog(">    AndroidAccessory inited..");
	androidAccessory.isConnected = 0;
	androidAccessory.creds[ACCESSORY_MANUFACTURER] = manufacturer;
	androidAccessory.creds[ACCESSORY_MODEL] = model;
	androidAccessory.creds[ACCESSORY_DESCRIPTION] = description;
	androidAccessory.creds[ACCESSORY_VERSION] = version;
	androidAccessory.creds[ACCESSORY_URI] = uri;
	androidAccessory.creds[ACCESSORY_SERIAL] = serial;
	androidAccessory.audioSupport = isAudio;
}

uint8_t Is_AA_Connected(void) {
	return androidAccessory.isConnected;
}

USBH_StatusTypeDef USBH_AA_Process(USBH_HandleTypeDef *phost) {
	USBH_StatusTypeDef status = USBH_BUSY ;

	switch ((HOST_AndroidAccessoryStateTypeDef) phost->gState) {
	case HOST_AA_CHECK_COMPATIBILITY:
		USBH_UsrLog(">    HOST_AA_CHECK_COMPATIBILITY..");
		vendorID = phost->device.DevDesc.idVendor;
		productID = phost->device.DevDesc.idProduct;
		USBH_UsrLog(">    vid: %04X and pid: %04X", vendorID, productID);
		if (vendorID != 0) {
			if (ANDROID_ACCESSSORY_VENDOR_ID == vendorID
					&& (productID == USB_ACCESSORY_PRODUCT_ID
							|| productID == USB_ACCESSORY_ADB_PRODUCT_ID
							|| productID == USB_AUDIO_PRODUCT_ID
							|| productID == USB_AUDIO_ADB_PRODUCT_ID
							|| productID == USB_ACCESSORY_AUDIO_PRODUCT_ID
							|| productID == USB_ACCESSORY_AUDIO_ADB_PRODUCT_ID)) {
				androidAccessory.isConnected = 1;
				phost->gState = HOST_CHECK_CLASS;
			} else {
				phost->gState = HOST_AA_GET_PROTOCOL;
			}
		}
		break;
	case HOST_AA_GET_PROTOCOL:
		USBH_UsrLog(">    HOST_AA_GET_PROTOCOL..");
		if (USBH_GetAAProtocol(phost, protocolVersion, 2) == USBH_OK) {
			if (protocolVersion[0] > 0) {
				USBH_UsrLog(">    AoAv%d protocol version supported..",
						protocolVersion[0]);
				androidAccessory.credState = 0;
				phost->gState = HOST_AA_SET_CRED_STATE;
			}
		}
		break;
	case HOST_AA_SET_CRED_STATE:
		USBH_UsrLog(">    HOST_AA_SET_CRED_STATE..");
		if (androidAccessory.credState <= ACCESSORY_SERIAL) {
			if (USBH_SendAAString(phost, androidAccessory.credState,
					androidAccessory.creds[androidAccessory.credState])
					== USBH_OK) {
				androidAccessory.credState++;
			}
		} else {
			androidAccessory.credState = 0;
			if (androidAccessory.audioSupport == 1 && protocolVersion[0] > 1) {
				phost->gState = HOST_AA_SET_AUDIO;
			} else {
				phost->gState = HOST_AA_START_MODE;
			}
		}
		break;
	case HOST_AA_SET_AUDIO:
		if (USBH_AAEnableAudio(phost) == USBH_OK) {
			phost->gState = HOST_AA_START_MODE;
		}
		break;
	case HOST_AA_START_MODE:
		USBH_UsrLog(">    HOST_AA_START_MODE..");
		if (USBH_StartAccessory(phost) == USBH_OK) {
			USBH_UsrLog(">    Accessory mode on..");
			HAL_Delay(1000);
			phost->gState = HOST_AA_IDLE;
			MX_USB_HOST_Init();
		}
		break;
	default:
		break;
	}
	return status;
}


USBH_StatusTypeDef USBH_GetAAProtocol(USBH_HandleTypeDef *phost, uint8_t* buff,
		uint8_t length) {
	if (phost->RequestState == CMD_SEND) {
		phost->Control.setup.b.bmRequestType =
				USB_D2H | USB_REQ_TYPE_VENDOR | USB_REQ_RECIPIENT_DEVICE;
		phost->Control.setup.b.bRequest = ACCESSORY_GET_PROTOCOL;
		phost->Control.setup.b.wValue.w = 0;
		phost->Control.setup.b.wIndex.w = 0;
		phost->Control.setup.b.wLength.w = length;
	}
	return USBH_CtlReq(phost, buff, length);
}

USBH_StatusTypeDef USBH_SendAAString(USBH_HandleTypeDef *phost, uint8_t index,
		const char *str) {
	if (phost->RequestState == CMD_SEND) {
		phost->Control.setup.b.bmRequestType =
				USB_H2D | USB_REQ_TYPE_VENDOR | USB_REQ_RECIPIENT_DEVICE;
		phost->Control.setup.b.bRequest = ACCESSORY_SEND_STRING;
		phost->Control.setup.b.wValue.w = 0;
		phost->Control.setup.b.wIndex.w = index;
		phost->Control.setup.b.wLength.w = strlen((char*) str) + 1;
	}
	return USBH_CtlReq(phost, (uint8_t*) str, strlen(str) + 1);
}

USBH_StatusTypeDef USBH_StartAccessory(USBH_HandleTypeDef *phost) {
	if (phost->RequestState == CMD_SEND) {
		phost->Control.setup.b.bmRequestType =
				USB_H2D | USB_REQ_TYPE_VENDOR | USB_REQ_RECIPIENT_DEVICE;
		phost->Control.setup.b.bRequest = ACCESSORY_START;
		phost->Control.setup.b.wValue.w = 0;
		phost->Control.setup.b.wIndex.w = 0;
		phost->Control.setup.b.wLength.w = 0;
	}
	return USBH_CtlReq(phost, NULL, 0);
}

USBH_StatusTypeDef USBH_AAEnableAudio(USBH_HandleTypeDef *phost) {
	if (phost->RequestState == CMD_SEND) {
		phost->Control.setup.b.bmRequestType =
				USB_H2D | USB_REQ_TYPE_VENDOR | USB_REQ_RECIPIENT_DEVICE;
		phost->Control.setup.b.bRequest = ACCESSORY_SET_AUDIO_MODE;
		phost->Control.setup.b.wValue.w = 1;
		phost->Control.setup.b.wIndex.w = 0;
		phost->Control.setup.b.wLength.w = 0;
	}
	return USBH_CtlReq(phost, NULL, 0);
}

extern uint8_t rx_txBuffer[USB_RX_TX_BUFFER_SIZE];
extern uint16_t outSize;

static uint16_t USBH_ADK_read(USBH_HandleTypeDef *phost, uint8_t *buff,
		uint16_t len) {
	uint32_t xfercount;
	AA_HandleTypeDef *AA_Handle =
			(AA_HandleTypeDef *) phost->pActiveClass->pData;
	xfercount = USBH_LL_GetLastXferSize(phost, AA_Handle->InPipe);
	if (xfercount > 0) {
		int size = (xfercount > USB_RX_TX_BUFFER_SIZE) ? USB_RX_TX_BUFFER_SIZE : xfercount;
		memcpy(buff, rx_txBuffer, size);
		memset(rx_txBuffer, 0, size);
	}
	return (uint16_t) xfercount;
}

static USBH_StatusTypeDef USBH_ADK_write(USBH_HandleTypeDef *phost,
		uint8_t *buff, uint16_t len) {
	memcpy(rx_txBuffer, buff,
			(len > USB_RX_TX_BUFFER_SIZE) ? USB_RX_TX_BUFFER_SIZE : len);
	outSize = len;
	return USBH_OK;
}

uint8_t msg[USB_RX_TX_BUFFER_SIZE];
uint16_t len;

static void checkState(HOST_StateTypeDef state) {
	static HOST_StateTypeDef oldState;
	if(oldState != state) {
		char* s;
		switch(state) {
		case HOST_IDLE:
			s = "HOST_IDLE";
			break;
		case HOST_DEV_WAIT_FOR_ATTACHMENT:
			s = "HOST_DEV_WAIT_FOR_ATTACHMENT";
			break;
		case HOST_DEV_ATTACHED:
			s = "HOST_DEV_ATTACHED";
			break;
		case HOST_DEV_DISCONNECTED:
			s = "HOST_DEV_DISCONNECTED";
			break;
		case HOST_DETECT_DEVICE_SPEED:
			s = "HOST_DETECT_DEVICE_SPEED";
			break;
		case HOST_ENUMERATION:
			s = "HOST_ENUMERATION";
			break;
		case HOST_CLASS_REQUEST:
			s = "HOST_CLASS_REQUEST";
			break;
		case HOST_INPUT:
			s = "HOST_INPUT";
			break;
		case HOST_SET_CONFIGURATION:
			s = "HOST_SET_CONFIGURATION";
			break;
		case HOST_CHECK_CLASS:
			s = "HOST_CHECK_CLASS";
			break;
		case HOST_CLASS:
			s = "HOST_CLASS";
			break;
		case HOST_SUSPENDED:
			s = "HOST_SUSPENDED";
			break;
		case HOST_ABORT_STATE:
			s = "HOST_ABORT_STATE";
			break;
		case HOST_AA_IDLE:
			s = "HOST_AA_IDLE";
			break;
		case HOST_AA_CHECK_COMPATIBILITY:
			s = "HOST_AA_CHECK_COMPATIBILITY";
			break;
		case HOST_AA_GET_PROTOCOL:
			s = "HOST_AA_GET_PROTOCOL";
			break;
		case HOST_AA_SET_CRED_STATE:
			s = "HOST_AA_SET_CRED_STATE";
			break;
		case HOST_AA_SET_AUDIO:
			s = "HOST_AA_SET_AUDIO";
			break;
		case HOST_AA_START_MODE:
			s = "HOST_AA_START_MODE";
			break;
		default:
			s = "Не ведома зверюшка!";
			break;
		}
		USBH_DbgLog("> --MX_USBH_AA_Process gState: %s", s);
		oldState = state;
	}
};

void MX_USBH_AA_Process(USBH_HandleTypeDef *phost) {
	if (!Is_AA_Connected() && phost->gState >= HOST_CHECK_CLASS
			&& (int) phost->gState < (int) HOST_AA_CHECK_COMPATIBILITY) {
		phost->gState = HOST_AA_CHECK_COMPATIBILITY;
	}
	USBH_AA_Process(phost);

	// Accessory mode включен. Создается постоянный двунаправленный поток
	// обмена. Если он обрывается, приложение андроид закрывается, модем ребутит
	// USB. Если с первого ребута аксессуар не подключается, то со второго
	// уверенно.

	checkState(phost->gState);

	if (phost->gState == HOST_CLASS) {

#if 0
		static uint8_t oldPacket[MSG_BUF_SIZE];
		len = USBH_ADK_read(phost, msg, sizeof(msg));
		if (len > 0) {
			if (oldPacket[0] != msg[0] && oldPacket[1] != msg[1]) {
				oldPacket[0] = msg[0];
				oldPacket[1] = msg[1];
				if(msg[0] != 0x11 && msg[0] != 0x01) {
					USBH_UsrLog(">    --------receive: [0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x]",
							msg[0], msg[1], msg[2], msg[3], msg[4], msg[5]);
				}
				if(msg[0] == 0x03 && msg[1] != 0x00) {
					uint8_t r[2];
					r[0] = 0x06;
					r[1] = msg[1];
					USBH_Ibox_ReceiveCallback(r, MSG_BUF_SIZE);
				}
				counter++;
			}
		}

		uint8_t ack[1];
		if(msg[0] == 0x11) {
			ack[0] = 0x12;
			USBH_UsrLog(">    --------sent ack: [0x%02x]", ack[0]);
			USBH_ADK_write(phost, ack, 1);
		}

		// 0x3, 0x1 - подтверждение

		/*
		msg[0] = ((msg[0] == header && msg[1] == 10) || (header == 0x0))? 0x1: header;
		msg[1] = 0;
		USBH_ALL_TransmitRequest(msg, MSG_BUF_SIZE);
		if(header != msg[0]) {
			USBH_UsrLog(">    --------header = 0x%02x, msg[0] = 0x%02x", header, msg[0]);
			header = msg[0];
			USBH_UsrLog(">    --------Set header = 0x%02x, msg[0] = 0x%02x", header, msg[0]);
			USBH_Ibox_TransmitCallback(phost);
		}

		USBH_ADK_write(phost, msg, sizeof(msg));
		 */

#else

		// На каждый запрос должен быть обязательно ответ, иначе не взведется флаг
		// чтения и обмена не будет
		len = USBH_ADK_read(phost, msg, sizeof(msg));
		if (len > 0 && msg[0] != 0x0) {
			if(msg[0] == 0x13) {
				USBH_UsrLog(">    --------<<");
			} else {
				// Не обрабатываем ответ на пинг от андроида
				USBH_UsrLog(">    --------receive: [0x%02x, 0x%02x], len: [%d]", msg[0], msg[1], len);
				USBH_Ibox_ReceiveCallback(msg, len);
			}
		}

		if(msg[0] == 0x13 || msg[0] == 0x06 || msg[0] == 0x04) {
			msg[1] = msg[0];
			msg[0] = 0x14;
			USBH_UsrLog(">    --------sent ok: [0x%02x], {%s}", msg[0], msg);
			USBH_ADK_write(phost, msg, 2);
		}

		memset(msg, 0, sizeof(msg));
		len = USBH_ALL_TransmitRequest(msg, sizeof(msg));
		if(len > 0) {
			USBH_UsrLog(">    --------sent command: [0x%02x]", msg[0]);
			USBH_ADK_write(phost, msg, len);
			memset(msg, 0, len);
		}


#endif
	}
}

/**
 * Перезагрузка USB для обновления пайпов после
 * потери соединения с программой.
 * Внимание: hUsbHostFS - локальный
 */
void MX_USB_HOST_Reload(void) {
	MX_USB_HOST_Init();
}

/*
 * Background task
 */
void MX_USB_HOST_Process(void) {
	/* USB Host Background task */
	USBH_Process(&hUsbHostFS);
}

/**
 * Init USB host library, add supported class and start the library
 * @retval None
 */
void MX_USB_HOST_Init(void) {
	AndroidAccessory("ammlab.org", "HelloADK",
			"HelloADK for GR-SAKURA for STM32F4", "1.0",
			"https://play.google.com/store/apps/details?id=org.ammlab.android.helloadk",
			"1234567", 0);
	// TODO:
	//	AndroidAccessory("TDS", "Ephor", "Ephor telemetry box", "1.0",
	//			"https://play.google.com/store/apps/details?id=org.ammlab.android.helloadk", "1234567", 0);
	USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS);
	USBH_Start(&hUsbHostFS);
	USBH_RegisterClass(&hUsbHostFS, USBH_IBOX_CLASS);
}

/*
 * user callback definition
 */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id) {
	/* USER CODE BEGIN CALL_BACK_1 */
	switch (id) {
	case HOST_USER_SELECT_CONFIGURATION:
		USBH_UsrLog(">    ----USB select configuration..");
		break;

	case HOST_USER_DISCONNECTION:
		USBH_UsrLog(">    ----USB disconnection..");
		Appli_state = APPLICATION_DISCONNECT;
		break;

	case HOST_USER_CLASS_ACTIVE:
		USBH_UsrLog(">    ----USB active..");
		Appli_state = APPLICATION_READY;
		break;

	case HOST_USER_CONNECTION:
		USBH_UsrLog(">    ----USB connected..");
		Appli_state = APPLICATION_START;
		break;

	default:
		break;
	}
	/* USER CODE END CALL_BACK_1 */
}

