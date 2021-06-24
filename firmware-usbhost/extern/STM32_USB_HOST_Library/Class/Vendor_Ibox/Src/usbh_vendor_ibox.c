#include "config.h"
#ifdef DEVICE_USB
/* Includes ------------------------------------------------------------------*/
#include "usbh_vendor_ibox.h"
#include "common/platform/include/platform.h"

/*
 * Data read call back
 */
void (*usbDataReceivedCallback)(uint8_t*, int);

uint32_t USBH_ALL_TransmitRequest(uint8_t *writeData, const uint32_t maxLen);

static USBH_StatusTypeDef USBH_AA_InterfaceInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_AA_InterfaceDeInit(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_AA_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_AA_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_AA_SOFProcess(USBH_HandleTypeDef *phost);

uint8_t rx_txBuffer[USB_RX_TX_BUFFER_SIZE];
uint16_t outSize = 0;

USBH_ClassTypeDef Vendor_Ibox_Class = {
		"ANDROID ACCESSORY",
		USB_AA_CLASS_CODE,
		USBH_AA_InterfaceInit,
		USBH_AA_InterfaceDeInit,
		USBH_AA_ClassRequest,
		USBH_AA_Process,
		USBH_AA_SOFProcess,
		NULL };

static USBH_StatusTypeDef USBH_AA_InterfaceInit(USBH_HandleTypeDef *phost) {
	USBH_UsrLog(">    ----USBH_AA_InterfaceInit..");
	uint8_t interface = 0xFF;
	AA_HandleTypeDef *AA_Handle;
	USBH_StatusTypeDef status = USBH_FAIL;

	interface = USBH_FindInterface(phost,
			phost->pActiveClass->ClassCode,
			USB_AA_CLASS_CODE,
			USB_AA_SUB_CLASS_CODE);

	if (interface == 0xFF) /* Not Valid Interface */
	{
		USBH_UsrLog(">    ----Cannot Find the interface for %s class..",
				phost->pActiveClass->Name);
		status = USBH_FAIL;
	} else {
		USBH_SelectInterface(phost, interface);
		phost->pActiveClass->pData = (AA_HandleTypeDef *) USBH_malloc(
				sizeof(AA_HandleTypeDef));
		AA_Handle = (AA_HandleTypeDef *) phost->pActiveClass->pData;

		if (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress
				& 0x80) {
			AA_Handle->InEp =
					(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress);
			AA_Handle->InEpSize =
					phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].wMaxPacketSize;
		} else {
			AA_Handle->OutEp =
					(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress);
			AA_Handle->OutEpSize =
					phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].wMaxPacketSize;
		}

		if (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].bEndpointAddress
				& 0x80) {
			AA_Handle->InEp =
					(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].bEndpointAddress);
			AA_Handle->InEpSize =
					phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].wMaxPacketSize;
		} else {
			AA_Handle->OutEp =
					(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].bEndpointAddress);
			AA_Handle->OutEpSize =
					phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].wMaxPacketSize;
		}

		AA_Handle->OutPipe = USBH_AllocPipe(phost, AA_Handle->OutEp);
		AA_Handle->InPipe = USBH_AllocPipe(phost, AA_Handle->InEp);

		/* Open the new channels */
		USBH_OpenPipe(phost, AA_Handle->OutPipe, AA_Handle->OutEp,
				phost->device.address, phost->device.speed,
				USB_EP_TYPE_BULK, AA_Handle->OutEpSize);

		USBH_OpenPipe(phost, AA_Handle->InPipe, AA_Handle->InEp,
				phost->device.address, phost->device.speed,
				USB_EP_TYPE_BULK, AA_Handle->InEpSize);

		USBH_LL_SetToggle(phost, AA_Handle->InPipe, 0);
		USBH_LL_SetToggle(phost, AA_Handle->OutPipe, 0);
		status = USBH_OK;
	}
	return status;
}

static USBH_StatusTypeDef USBH_AA_InterfaceDeInit(USBH_HandleTypeDef *phost) {
	USBH_UsrLog(">    ----USBH_AA_InterfaceDeInit.");
	AA_HandleTypeDef *AA_Handle =
			(AA_HandleTypeDef *) phost->pActiveClass->pData;

	if (AA_Handle->OutPipe) {
		USBH_ClosePipe(phost, AA_Handle->OutPipe);
		USBH_FreePipe(phost, AA_Handle->OutPipe);
		AA_Handle->OutPipe = 0; /* Reset the Channel as Free */
	}

	if (AA_Handle->InPipe) {
		USBH_ClosePipe(phost, AA_Handle->InPipe);
		USBH_FreePipe(phost, AA_Handle->InPipe);
		AA_Handle->InPipe = 0; /* Reset the Channel as Free */
	}

	if (phost->pActiveClass->pData) {
		USBH_free(phost->pActiveClass->pData);
		phost->pActiveClass->pData = 0;
	}
	return USBH_OK;
}

uint8_t write = 0;

static USBH_StatusTypeDef USBH_AA_ClassRequest(USBH_HandleTypeDef *phost) {
	USBH_UsrLog(">    ----USBH_AA_ClassRequest");
	AA_HandleTypeDef *AA_Handle =
			(AA_HandleTypeDef *) phost->pActiveClass->pData;
	USBH_UsrLog(">    eIn: %02x eOut: %02x", AA_Handle->InEp, AA_Handle->OutEp);
	USBH_UsrLog(">    szIn: %d szOut: %d", AA_Handle->InEpSize, AA_Handle->OutEpSize);
	AA_Handle->state = AA_READ;
	return USBH_OK;
}

void accesoryWriteUSBDataFromRxTxBuffer(void) {
	write = 1;
}

static void checkStateOfStatus(AA_StateTypeDef state) {
	static oldState;
	if(oldState != state) {
		char* s;
		switch(state) {
		case AA_IDLE:
			s = "AA_IDLE";
			break;
		case AA_READ:
			s = "AA_READ";
			break;
		case AA_READ_WAIT:
			s = "AA_READ_WAIT";
			break;
		case AA_WRITE:
			s = "AA_WRITE";
			break;
		case AA_WRITE_WAIT:
			s = "AA_WRITE_WAIT";
			break;
		case AA_BUSY:
			s = "AA_BUSY";
			break;
		default:
			s = "Х.. ево знает!!";
			break;
		}
		//		USBH_DbgLog(">    ----AA Sate: %s", s);
		oldState = state;
	}

}

static USBH_StatusTypeDef USBH_AA_Process(USBH_HandleTypeDef *phost) {
	AA_HandleTypeDef *AA_Handle =
			(AA_HandleTypeDef *) phost->pActiveClass->pData;

#if 0
	USBH_URBStateTypeDef URB_Status = USBH_URB_IDLE;

	if (write || USBH_ALL_TransmitRequest(rx_txBuffer, USB_RX_TX_BUFFER_SIZE) > 0) {
		AA_Handle->state = AA_WRITE;
		write = 0;
	}

	switch (AA_Handle->state) {
	case AA_READ:
		if (USBH_BulkReceiveData(phost, rx_txBuffer, USB_RX_TX_BUFFER_SIZE,
				AA_Handle->InPipe) != USBH_OK) {
			USBH_DbgLog("Read error");
		}
		AA_Handle->state = AA_READ_WAIT;
		break;
	case AA_READ_WAIT:
		URB_Status = USBH_LL_GetURBState(phost, AA_Handle->InPipe);
		if (URB_Status == USBH_URB_DONE) {
			USBH_Ibox_ReceiveCallback(rx_txBuffer, USB_RX_TX_BUFFER_SIZE);
			AA_Handle->state = AA_READ;
		}
		break;
	case AA_WRITE:
		if (USBH_LL_GetURBState(phost, AA_Handle->InPipe) == USBH_URB_DONE)  { // ?? почему out?
			USBH_DbgLog("Start write..");
			if (USBH_BulkSendData(phost, rx_txBuffer, USB_RX_TX_BUFFER_SIZE,
					AA_Handle->OutPipe, 1U) != USBH_OK) {
				USBH_DbgLog("Write error");
			} else {
				AA_Handle->state = AA_READ;
			}
		}
		break;
	case AA_WRITE_WAIT:
		URB_Status = USBH_LL_GetURBState(phost, AA_Handle->OutPipe);
		if (URB_Status == USBH_URB_DONE) {
			AA_Handle->state = AA_READ;
		} else if (URB_Status == USBH_URB_NOTREADY) {
			AA_Handle->state = AA_WRITE;
		}
		break;
	default:
		break;
	}
	return USBH_OK;
#else
	USBH_StatusTypeDef status = USBH_BUSY;
	USBH_URBStateTypeDef URB_Status;

	checkStateOfStatus(AA_Handle->state);

	switch (AA_Handle->state) {
	case AA_IDLE:
		AA_Handle->state = AA_WRITE;

	case AA_WRITE:
		if (outSize > 0) {
			// USBH_DbgLog(">    ----AA_WRITE: {%s}, %d", rx_txBuffer, outSize);
			USBH_BulkSendData(phost, rx_txBuffer, outSize,
					AA_Handle->OutPipe, 1U);
			memset(rx_txBuffer, 0, sizeof(rx_txBuffer));
			outSize = 0;
			AA_Handle->state = AA_READ;
		}
		break;

	case AA_READ:
		URB_Status = USBH_LL_GetURBState(phost, AA_Handle->InPipe);
		if (URB_Status > URB_DONE) {
			break;
		}
		USBH_BulkReceiveData(phost, rx_txBuffer, USB_RX_TX_BUFFER_SIZE,
				AA_Handle->InPipe);
		AA_Handle->state = AA_IDLE;
		break;

	case AA_BUSY:
		AA_Handle->state = AA_IDLE;
		outSize = 0;
		break;

	default:
		break;
	}
	status = USBH_OK;
	return status;
#endif
}

static USBH_StatusTypeDef USBH_AA_SOFProcess(USBH_HandleTypeDef *phost) {
	return USBH_OK;
}

#endif
