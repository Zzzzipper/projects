#ifndef ANDROIDACCESSORY_H_
#define ANDROIDACCESSORY_H_

#include "usbh_core.h"

#if defined __cplusplus
extern "C" {
#endif

#define ANDROID_ACCESSSORY_VENDOR_ID			0x18D1

//AOAv1
#define USB_ACCESSORY_PRODUCT_ID				0x2D00
#define USB_ACCESSORY_ADB_PRODUCT_ID			0x2D01

//AOAv2
#define USB_AUDIO_PRODUCT_ID					0x2D02
#define USB_AUDIO_ADB_PRODUCT_ID				0x2D03
#define USB_ACCESSORY_AUDIO_PRODUCT_ID			0x2D04
#define USB_ACCESSORY_AUDIO_ADB_PRODUCT_ID		0x2D05


#define ACCESSORY_GET_PROTOCOL					51
#define ACCESSORY_SEND_STRING					52
#define ACCESSORY_START							53

//AOAv2 SET_AUDIO_MODE
#define ACCESSORY_SET_AUDIO_MODE				58

typedef enum {
	HOST_AA_IDLE = 100,
	HOST_AA_CHECK_COMPATIBILITY,
	HOST_AA_GET_PROTOCOL,
	HOST_AA_SET_CRED_STATE,
	HOST_AA_SET_AUDIO,
	HOST_AA_START_MODE,
} HOST_AndroidAccessoryStateTypeDef;

typedef enum {
	ACCESSORY_MANUFACTURER = 0,
	ACCESSORY_MODEL,
	ACCESSORY_DESCRIPTION,
	ACCESSORY_VERSION,
	ACCESSORY_URI,
	ACCESSORY_SERIAL
} HOST_AACredentialsStateTypeDef;

typedef struct {
	const char *creds[6];
	uint8_t credState;
	uint8_t audioSupport;
	uint8_t isConnected;
} AA;

/** Status of the application. */
typedef enum {
	APPLICATION_IDLE = 0,
	APPLICATION_START,
	APPLICATION_READY,
	APPLICATION_DISCONNECT
} ApplicationTypeDef;

void AndroidAccessory(const char *manufacturer, const char *model, const char *description,
		const char *version, const char *uri, const char *serial, uint8_t isAudioSupported);

USBH_StatusTypeDef USBH_AA_CheckStatus(USBH_HandleTypeDef *phost);

USBH_StatusTypeDef USBH_AA_Process(USBH_HandleTypeDef *phost);

void MX_USBH_AA_Process(USBH_HandleTypeDef *phost);
void MX_USB_HOST_Process(void);
void MX_USB_HOST_Init(void);
void MX_USB_HOST_Reload(void);

void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed);

typedef void (*StartUsbCallback)();
void registerStartUsbCallback(StartUsbCallback func_);
extern StartUsbCallback startUsbCallback;

/* Private */
/* --------------------------------------------------------------------------- */
USBH_StatusTypeDef USBH_GetAAProtocol(USBH_HandleTypeDef *phost, uint8_t* buff,
		uint8_t length);

USBH_StatusTypeDef USBH_SendAAString(USBH_HandleTypeDef *phost, uint8_t index,
		const char *str);

USBH_StatusTypeDef USBH_StartAccessory(USBH_HandleTypeDef *phost);

USBH_StatusTypeDef USBH_AAEnableAudio(USBH_HandleTypeDef *phost);
/* --------------------------------------------------------------------------- */

#if defined __cplusplus
}
#endif

#endif /* ANDROIDACCESSORY_H_ */
