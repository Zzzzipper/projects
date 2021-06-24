#include "config.h"
#ifdef DEVICE_USB

#ifndef _USBH_VENDOR_IBOX_H
#define _USBH_VENDOR_IBOX_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "usbh_core.h"

extern USBH_ClassTypeDef	Vendor_Ibox_Class;
#define USBH_IBOX_CLASS		&Vendor_Ibox_Class

#define USB_AA_CLASS_CODE		0xFF
#define USB_AA_SUB_CLASS_CODE	0xFF
#define USB_AA_PROTOCOL			0

typedef enum {
	AA_IDLE = 0,
	AA_READ,
	AA_READ_WAIT,
	AA_WRITE,
	AA_WRITE_WAIT,
	AA_BUSY,
} AA_StateTypeDef;

typedef struct {
	AA_StateTypeDef state;
	uint8_t InPipe;
	uint8_t OutPipe;
	uint8_t OutEp;
	uint8_t InEp;
	uint16_t OutEpSize;
	uint16_t InEpSize;
} AA_HandleTypeDef;

#define USB_RX_TX_BUFFER_SIZE		64

/*
 * Write data to @rx_txBuffer and
 * call it when @rx_txBuffer is ready for transmission
 */
void accesoryWriteUSBDataFromRxTxBuffer(void);

#ifdef __cplusplus
}
#endif

#endif // _USBH_VENDOR_IBOX_H
#endif // DEVICE_USB
