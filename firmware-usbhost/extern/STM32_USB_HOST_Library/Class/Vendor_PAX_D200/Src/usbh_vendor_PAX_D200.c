#include "config.h"
#ifdef DEVICE_USB
/* Includes ------------------------------------------------------------------*/
#include "usbh_vendor_PAX_D200.h"
#include "common/platform/include/platform.h"



#define USBH_PAX_D200_BUFFER_SIZE                 1024


static USBH_StatusTypeDef PAX_D200_InterfaceInit  (USBH_HandleTypeDef *phost);

static USBH_StatusTypeDef PAX_D200_InterfaceDeInit  (USBH_HandleTypeDef *phost);

static USBH_StatusTypeDef PAX_D200_Process(USBH_HandleTypeDef *phost);

static USBH_StatusTypeDef PAX_D200_SOFProcess(USBH_HandleTypeDef *phost);

static USBH_StatusTypeDef PAX_D200_ClassRequest (USBH_HandleTypeDef *phost);

static USBH_StatusTypeDef PAX_D200_ProcessTransmission(USBH_HandleTypeDef *phost);

static USBH_StatusTypeDef PAX_D200_ProcessReception(USBH_HandleTypeDef *phost);

static void PAX_D200_PrepareDataHeader(USBH_HandleTypeDef *phost, ST_BULK_IO *bioPack, uint8_t reqType, uint16_t len);

static USBH_StatusTypeDef  PAX_D200_PrepareTransmit(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint32_t length);

static USBH_StatusTypeDef  PAX_D200_PrepareReceive(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint32_t length);

static void PAX_D200_Reset(USBH_HandleTypeDef *phost);

static USBH_StatusTypeDef CheckStatus(USBH_HandleTypeDef *phost, USBH_StatusTypeDef result);

extern void HAL_Delay(uint32_t delay);
extern void HAL_Delay_us(uint32_t delay);

//======================================================================================
// ======================== Из примера Linux драйвера, ttyPos.h ========================
#define PAX_D200_WRITE_COMMAND			0	/* write to usb device command */
#define PAX_D200_READ_COMMAND			1	/* read from to usb device command */
#define PAX_D200_STATUS_COMMAND			2	/* get device buffer status command */
#define PAX_D200_RESET_COMMAND       	3
#define PAX_D200_MAXDATA_COMMAND     	4

#define MAX_TRANSFER_SIZE 				512
#define MAX_RETRY_S						1

static int PAX_D200_VerifyChecksum(ST_BULK_IO *p_bio);
static void PAX_D200_SetChecksum(ST_BULK_IO *p_bio);
static unsigned char PAX_D200_GetXOR(uint8_t *buf, unsigned int len);
static USBH_StatusTypeDef PAX_D200_ProcessCommand(USBH_HandleTypeDef *phost, ST_BULK_IO *bioPack);

static ST_BULK_IO PAX_D200_DATA;
//======================================================================================

USBH_ClassTypeDef  Vendor_PAX_D200_Class =
{
  "D200",
  VENDOR_SPECIFIC,
  PAX_D200_InterfaceInit,
  PAX_D200_InterfaceDeInit,
  PAX_D200_ClassRequest,
  PAX_D200_Process,
  PAX_D200_SOFProcess,
  NULL,
  USB_PID_D200,
  USB_VID_D200,
};

/**
  * @brief  USBH_PAX_D200_InterfaceInit
  *         The function init the PAX_D200 class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef PAX_D200_InterfaceInit(USBH_HandleTypeDef *phost)
{

	USBH_StatusTypeDef status = USBH_FAIL;
	uint8_t interface;
	PAX_D200_HandleTypeDef *handle;

	interface = 0;

	USBH_SelectInterface(phost, interface);
	phost->pActiveClass->pData = (PAX_D200_HandleTypeDef *) USBH_malloc(sizeof(PAX_D200_HandleTypeDef));
	handle = (PAX_D200_HandleTypeDef*) phost->pActiveClass->pData;


	/*Collect the class specific endpoint address and length*/
	if (phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress & 0x80)
	{
		handle->DataItf.InEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress;
		handle->DataItf.InEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
	}
	else
	{
		handle->DataItf.OutEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].bEndpointAddress;
		handle->DataItf.OutEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[0].wMaxPacketSize;
	}

	if (phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress & 0x80)
	{
		handle->DataItf.InEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress;
		handle->DataItf.InEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].wMaxPacketSize;
	}
	else
	{
		handle->DataItf.OutEp = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].bEndpointAddress;
		handle->DataItf.OutEpSize = phost->device.CfgDesc.Itf_Desc[interface].Ep_Desc[1].wMaxPacketSize;
	}

	/*Allocate the length for host channel number out*/
	handle->DataItf.OutPipe = USBH_AllocPipe(phost, handle->DataItf.OutEp);

	/*Allocate the length for host channel number in*/
	handle->DataItf.InPipe = USBH_AllocPipe(phost, handle->DataItf.InEp);

	/* Open channel for OUT endpoint */
	USBH_OpenPipe(phost, handle->DataItf.OutPipe, handle->DataItf.OutEp, phost->device.address,
			phost->device.speed, USB_EP_TYPE_BULK, handle->DataItf.OutEpSize);
	/* Open channel for IN endpoint */
	USBH_OpenPipe(phost, handle->DataItf.InPipe, handle->DataItf.InEp, phost->device.address,
			phost->device.speed, USB_EP_TYPE_BULK, handle->DataItf.InEpSize);


	USBH_LL_SetToggle(phost, handle->DataItf.OutPipe, 0);
	USBH_LL_SetToggle(phost, handle->DataItf.InPipe, 0);
	status = USBH_OK;

	PAX_D200_Reset(phost);
	handle->state = PAX_D200_IDLE_STATE;
	handle->retriesOfReset = 0;

	return status;
}



/**
  * @brief  USBH_PAX_D200_InterfaceDeInit
  *         The function DeInit the Pipes used for the PAX_D200 class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
USBH_StatusTypeDef PAX_D200_InterfaceDeInit (USBH_HandleTypeDef *phost)
{
  PAX_D200_HandleTypeDef *handle =  (PAX_D200_HandleTypeDef*) phost->pActiveClass->pData;
  
  if ( handle->DataItf.InPipe)
  {
    USBH_ClosePipe(phost, handle->DataItf.InPipe);
    USBH_FreePipe  (phost, handle->DataItf.InPipe);
    handle->DataItf.InPipe = 0;     /* Reset the Channel as Free */
  }
  
  if ( handle->DataItf.OutPipe)
  {
    USBH_ClosePipe(phost, handle->DataItf.OutPipe);
    USBH_FreePipe  (phost, handle->DataItf.OutPipe);
    handle->DataItf.OutPipe = 0;     /* Reset the Channel as Free */
  } 
  
  if(phost->pActiveClass->pData)
  {
    USBH_free (phost->pActiveClass->pData);
    phost->pActiveClass->pData = 0;
  }
   
  return USBH_OK;
}

/**
  * @brief  USBH_PAX_D200_ClassRequest
  *         The function is responsible for handling Standard requests
  *         for PAX_D200 class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef PAX_D200_ClassRequest(USBH_HandleTypeDef *phost)
{
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef *) phost->pActiveClass->pData;
	handle->state = PAX_D200_RESET_STATE;
	return USBH_OK;
}

/**
  * @brief  USBH_PAX_D200_Process
  *         The function is for managing state machine for PAX_D200 data transfers
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef PAX_D200_Process(USBH_HandleTypeDef *phost)
{
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef *) phost->pActiveClass->pData;
	ST_BULK_IO *bioPack = &PAX_D200_DATA;

	if (handle->data_tx_state != PAX_D200_IDLE_DATA_STATE)
	{
		return CheckStatus(phost, PAX_D200_ProcessTransmission(phost));
	}

	if (handle->data_rx_state != PAX_D200_IDLE_DATA_STATE)
	{
		return CheckStatus(phost, PAX_D200_ProcessReception(phost));
	}

	if (handle->stateProcessCommand != PAX_D200_IDLE_PROCESS_COMMAND_STATE)
	{
		return CheckStatus(phost, PAX_D200_ProcessCommand(phost, bioPack));
	}

	USBH_StatusTypeDef result = USBH_BUSY;

	switch (handle->state)
	{
	case PAX_D200_IDLE_STATE:
	{
		result = USBH_OK;
	}
	break;

	case PAX_D200_FULL_RESET_STATE:
//	case PAX_D200_RESET_STATE:

		USBH_ErrLog("Ошибка связи по USB, перезапуск...");
		handle->state = PAX_D200_IDLE_STATE;
		//USBH_ReEnumerate(phost);

		return USBH_INTERNAL_RESET;
	break;

	case PAX_D200_RESET_STATE:
	{
		PAX_D200_PrepareDataHeader(phost, bioPack, PAX_D200_RESET_COMMAND, 0);

		handle->state = PAX_D200_RESET_WAIT_STATE;
		handle->data_rx_state = PAX_D200_IDLE_DATA_STATE;
		handle->data_tx_state = PAX_D200_IDLE_DATA_STATE;
		handle->stateProcessCommand = PAX_D200_SEND_ONLY_REQUEST;
	}
	break;

	case PAX_D200_RESET_WAIT_STATE:
	{
		handle->state = PAX_D200_CLEAR_FEATURE;
	}
	break;

	case PAX_D200_CLEAR_FEATURE:
	{
		uint8_t ep_addr   = phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress;
		USBH_StatusTypeDef req_status = USBH_ClrFeature(phost, ep_addr);

		if(req_status == USBH_OK )
		{
			/*Change the state to waiting*/
			handle->state = PAX_D200_SEND_CNTRL;
		}
	}
    break;

	case PAX_D200_SEND_CNTRL:
	{
		USBH_StatusTypeDef result = USBH_FAIL;
		/*
		 Позволяет драйверу отправлять и получать управляющие сообщения USB (http://dmilvdv.narod.ru/Translate/LDD3/ldd_usb_transfers_without_urbs.html)
		 usb_control_msg(pdx->udev,usb_sndctrlpipe(pdx->udev, 0),
		 0x01, // Значение USB запроса для управляющего сообщения
		 0x40, // Значение типа запроса USB для управляющего сообщения
		 0x300, // Значение USB сообщения для управляющего сообщения
		 0,	// Значение индекса сообщения USB для управляющего сообщения
		 0,	// Указатель на данные для отправки в устройство, если это ВЫХОДНАЯ оконечная точка. Если это ВХОДНАЯ оконечная точка, он указывает, где должны быть размещены данные после чтения с устройства
		 0,	// Размер буфера, на который указывает параметр data
		 5000	// Количество времени, в тиках, которое необходимо подождать до выхода. Если это значение равно 0, функция ждёт завершения сообщения бесконечно
		 );
		 */

		if (phost->RequestState == CMD_SEND)
		{
			phost->Control.setup.b.bRequest = 0x01;
			phost->Control.setup.b.bmRequestType = USB_REQ_TYPE_VENDOR;
			phost->Control.setup.b.wValue.w = 0x0300;
			phost->Control.setup.b.wIndex.w = 0;
			phost->Control.setup.b.wLength.w = 0;
		}


		result = USBH_CtlReq(phost, 0, 0);

		if (result == USBH_OK)
		{
			handle->state = PAX_D200_CHECK_MAX_DATA;//PAX_D200_CLEAR_FEATURE;
		}

		return result;
	}
	break;

	case PAX_D200_CHECK_MAX_DATA:
	{
		USBH_DbgLog("Устройство инициализировано, начинаем опрос");

		PAX_D200_PrepareDataHeader(phost, bioPack, PAX_D200_MAXDATA_COMMAND, 0);

		handle->state = PAX_D200_CHECK_MAX_DATA_WAIT;
		handle->stateProcessCommand = PAX_D200_SEND_REQUEST;
	}
	break;

	case PAX_D200_CHECK_MAX_DATA_WAIT:
	{
		int retval = 0;

		if (bioPack->len != 2)
		{
			retval = 430;
			USBH_ErrLog("Len!=2: %d\n", bioPack->len);
		}
		else if (bioPack->seqNo != handle->seqCount)
		{
			retval = 431;
			USBH_ErrLog("Error sequence, awaiting: %X, actual: %X\n", handle->seqCount, bioPack->seqNo);
		}
		else if (bioPack->reqType != PAX_D200_MAXDATA_COMMAND)
		{
			retval = 432;
			USBH_ErrLog("Error received command: %X\n", bioPack->reqType);
		}
		else if (bioPack->len == 0)
		{
			retval = 433;
			USBH_ErrLog("Empty received data\n");
		}

		if (retval)
		{
			handle->state = PAX_D200_FULL_RESET_STATE;
			return CheckStatus(phost, USBH_FAIL);
		}

		memcpy(&handle->maxdata, bioPack->data, sizeof(handle->maxdata));
		if (handle->maxdata > sizeof(bioPack->data))
			handle->maxdata = sizeof(bioPack->data);

		USBH_DbgLog("maxdata: %d\n", handle->maxdata);

		if (handle->maxdata > (uint16_t) PAX_D200_MAX_DATA)
		{
			USBH_ErrLog("received maxdata size(%d) > defined PAX_D200_MAX_DATA(%d)\n", handle->maxdata,
					PAX_D200_MAX_DATA);
		}

		handle->state = PAX_D200_CHECK_STATUS_COMMAND;
	}
	break;

	case PAX_D200_CHECK_STATUS_COMMAND:
	{
		/* get device buffer status */

		PAX_D200_PrepareDataHeader(phost, bioPack, PAX_D200_STATUS_COMMAND, 0);
		handle->state = PAX_D200_CHECK_STATUS_COMMAND_WAIT;
		handle->stateProcessCommand = PAX_D200_SEND_REQUEST;
	}
	break;

	case PAX_D200_CHECK_STATUS_COMMAND_WAIT:
	{
		int retval = 0;

		if (bioPack->len != 16)
		{
			retval = 130;
			USBH_ErrLog("Len != 16: %d\n", bioPack->len);
		}
		else if (bioPack->seqNo != handle->seqCount)
		{
			retval = 131;
			USBH_ErrLog("Error sequence, awaiting: %X, actual: %X\n", handle->seqCount, bioPack->seqNo);
		}
		else if (bioPack->reqType != PAX_D200_STATUS_COMMAND)
		{
			retval = 132;
			USBH_ErrLog("Error received command: %X\n", bioPack->reqType);
		}

		if (retval)
		{
			USBH_ErrLog("Error code: %d", retval);
			handle->state = PAX_D200_CHECK_STATUS_COMMAND;
			return CheckStatus(phost, USBH_FAIL);
		}

		memcpy(&handle->bioDevState, bioPack->data, sizeof(handle->bioDevState));

//			USBH_DbgLog("Received status, txBufSize: %d, rxBufSize: %d, txLeft: %d, rxLeft: %d", handle->bioDevState.txBufSize, handle->bioDevState.rxBufSize, handle->bioDevState.txLeft, handle->bioDevState.rxLeft);

		if (!handle->bioDevState.txLeft)
		{
			handle->state = PAX_D200_WRITE_DATA_COMMAND;
		}
		else
		{
			handle->rlen = handle->bioDevState.txLeft;
			if (handle->rlen > (handle->maxdata - 1))
				handle->rlen = handle->maxdata - 1;

			if (handle->rlen)
				handle->state = PAX_D200_READ_DATA_COMMAND;
			else
				handle->state = PAX_D200_WRITE_DATA_COMMAND;
		}
	}
	break;

	case PAX_D200_READ_DATA_COMMAND:
	{
		bioPack->data[0] = (unsigned short) handle->rlen & 0xff;
		bioPack->data[1] = (unsigned short) handle->rlen >> 8;

		PAX_D200_PrepareDataHeader(phost, bioPack, PAX_D200_READ_COMMAND, 2);
		handle->state = PAX_D200_READ_DATA_COMMAND_WAIT;
		handle->stateProcessCommand = PAX_D200_SEND_REQUEST;
	}
	break;

	case PAX_D200_READ_DATA_COMMAND_WAIT:
	{
		int retval = 0;

		if (bioPack->seqNo != handle->seqCount)
		{
			retval = 231;
			USBH_ErrLog("Error read sequence, awaiting: %X, actual: %X\n", handle->seqCount, bioPack->seqNo);
		}
		else if (bioPack->reqType != PAX_D200_READ_COMMAND)
		{
			if (bioPack->reqType == PAX_D200_STATUS_COMMAND && bioPack->len >= sizeof(handle->bioDevState))
			{
				memcpy(&handle->bioDevState, bioPack->data, sizeof(handle->bioDevState));
				handle->state = PAX_D200_WRITE_DATA_COMMAND; /* no data to fetch */
				return result;
			}

			USBH_ErrLog("Error read data command, seq: %X, ERROR req_type: %X\n", handle->seqCount, bioPack->reqType);
			retval = 232;
		}
		else if (bioPack->len > handle->rlen)
		{
			retval = 233;
			USBH_ErrLog("MORE DATA FETCHED THAN DECLARED, NEED: %d, RN: %d\n", (int ) handle->rlen, (int ) bioPack->len);
		}

		if (retval)
		{
			USBH_ErrLog("Error code: %d", retval);
			handle->state = PAX_D200_CHECK_STATUS_COMMAND;
			return CheckStatus(phost, USBH_FAIL);
		}

		handle->rlen = bioPack->len;
		handle->bioDevState.txLeft -= handle->rlen;
		USBH_PAX_D200_ReceiveCallback(phost, (uint8_t *) &bioPack->data, handle->rlen, handle->bioDevState.txLeft);

		if (handle->bioDevState.txLeft)
			handle->state = PAX_D200_READ_DATA_COMMAND;
		else
			handle->state = PAX_D200_CHECK_STATUS_COMMAND;
	}
	break;

	case PAX_D200_WRITE_DATA_COMMAND:
	{
		handle->wlen = USBH_ALL_TransmitRequest(bioPack->data, sizeof(bioPack->data));
		if (!handle->wlen)
		{
			handle->state = PAX_D200_CHECK_STATUS_COMMAND;
			return USBH_OK;
		}

		PAX_D200_PrepareDataHeader(phost, bioPack, PAX_D200_WRITE_COMMAND, handle->wlen);
		handle->state = PAX_D200_WRITE_DATA_COMMAND_WAIT;
		handle->stateProcessCommand = PAX_D200_SEND_REQUEST;
	}
	break;

	case PAX_D200_WRITE_DATA_COMMAND_WAIT:
	{
		int retval = 0;

		if (bioPack->seqNo != handle->seqCount)
		{
			retval = 331;
			USBH_ErrLog("Error write sequence, awaiting: %X, actual: %X\n", handle->seqCount, bioPack->seqNo);
		}
		else if (bioPack->reqType != PAX_D200_STATUS_COMMAND)
		{
			retval = 332;
			USBH_ErrLog("Error write data command, seq: %d, ERROR req_type: %d, len: %d\n", handle->seqCount, bioPack->reqType, bioPack->len);
		}

		if (retval)
		{
			USBH_ErrLog("Error code: %d", retval);
			handle->state = PAX_D200_CHECK_STATUS_COMMAND;
			return CheckStatus(phost, USBH_FAIL);
		}

		handle->state = PAX_D200_CHECK_STATUS_COMMAND;

		USBH_PAX_D200_TransmitCallback(phost);
	}
	break;

	default:
	break;

	}

	return result;
}

static USBH_StatusTypeDef PAX_D200_ProcessCommand(USBH_HandleTypeDef *phost, ST_BULK_IO *bioPack)
{
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef *) phost->pActiveClass->pData;

	switch (handle->stateProcessCommand)
	{
	case PAX_D200_SEND_ONLY_REQUEST:
	case PAX_D200_SEND_REQUEST:
	{
		/* stage 1: send command pack */
	    if(phost->device.DevDesc.bcdDevice >= 0x300)
	    {
	    	bioPack->len += 1;
	    	bioPack->data[bioPack->len - 1] = PAX_D200_GetXOR((uint8_t *) bioPack, bioPack->len - 1 + 4);
	    }
	    else 	PAX_D200_SetChecksum(bioPack);

	    PAX_D200_PrepareTransmit(phost, (uint8_t *) bioPack, bioPack->len + 4);

	    if (handle->stateProcessCommand == PAX_D200_SEND_ONLY_REQUEST)
	    	handle->stateProcessCommand = PAX_D200_IDLE_PROCESS_COMMAND_STATE;
	    else
	    	handle->stateProcessCommand = PAX_D200_RECEIVE_REQUEST;
	}
	break;

	case PAX_D200_RECEIVE_REQUEST:
	{
		/* stage 2: receive answer pack */

			/* clear pack flags */
		bioPack->seqNo = 0;
		bioPack->reqType = 0;
		bioPack->len = 0;
		handle->realReceivedLen = 0;

		PAX_D200_PrepareReceive(phost, (uint8_t *) bioPack, MAX_TRANSFER_SIZE);
	    handle->stateProcessCommand = PAX_D200_RECEIVE_REQUEST_WAIT;
	}
	break;

	case PAX_D200_RECEIVE_REQUEST_WAIT:
		if (phost->device.DevDesc.bcdDevice >= 0x300)
		{
			if (bioPack->len < 1)
				return USBH_FAIL;

			if (bioPack->len + 4 > handle->realReceivedLen)
			{
				if (bioPack->len + 4 > PAX_D200_MAX_DATA)
				{
//					USBH_ErrLog("Too big request data block: %d, resetting!", bioPack->len + 4);
					USBH_ErrLog("Too big request data block: %d!", bioPack->len + 4);
//					PAX_D200_Reset(phost);
//					break;
				}

				handle->pRxData += handle->DataItf.InEpSize;
				handle->data_rx_state = PAX_D200_RECEIVE_DATA;

				USBH_DbgLog("Request next data block, requested len: %d, real: %lu", bioPack->len + 4, handle->realReceivedLen);
				break;
			}

/*
 	uint8_t seqNo;
 	uint8_t reqType; // 0:OUT, 1:IN, 2:STAT, 3:RESET
 	uint16_t len;
 	uint8_t data[PAX_D200_MAX_DATA];
 */

			if (PAX_D200_GetXOR((uint8_t *) bioPack, bioPack->len + 4))
			{
				USBH_ErrLog("\r\nVERIFY CHECKSUM FAILED, seq: %d, type: %d, len: %d\r\n", bioPack->seqNo, bioPack->reqType, bioPack->len);

			#if (USBH_DEBUG_LEVEL > USB_LOG_LEVEL_DEBUG)
				printf("\r\n");

				for(uint16_t i = 0; i < (bioPack->len + 4); i++)
					printf("%2.2x ", ((uint8_t *) bioPack)[i]);

				printf("\r\n");
			#endif

				return USBH_FAIL;
			}
			bioPack->len -= 1;
		}
		else
		{
			USBH_ErrLog("Not implemented request for device id: %X", phost->device.DevDesc.bcdDevice);
			return USBH_NOT_SUPPORTED;
		}

		handle->stateProcessCommand = PAX_D200_IDLE_PROCESS_COMMAND_STATE;
	break;

	default:
	break;
	}

	return USBH_OK;
}

/**
  * @brief  USBH_PAX_D200_SOFProcess
  *         The function is for managing SOF callback 
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef PAX_D200_SOFProcess (USBH_HandleTypeDef *phost)
{
//	PROBE_TOGGLE(B, 3);

  return USBH_OK;  
}
                                   
  
/**
  * @brief  USBH_PAX_D200_Stop
  *         Stop current PAX_D200 Transmission
  * @param  phost: Host handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_PAX_D200_Stop(USBH_HandleTypeDef *phost)
{
  PAX_D200_HandleTypeDef *PAX_D200_Handle =  (PAX_D200_HandleTypeDef*) phost->pActiveClass->pData;
  
  if(phost->gState == HOST_CLASS)
  {
    PAX_D200_Handle->state = PAX_D200_IDLE_STATE;
    
    USBH_ClosePipe(phost, PAX_D200_Handle->DataItf.InPipe);
    USBH_ClosePipe(phost, PAX_D200_Handle->DataItf.OutPipe);
  }
  return USBH_OK;  
}

/**
 * @brief  This function return last received data size
 * @param  None
 * @retval None
 */
uint16_t USBH_PAX_D200_GetLastReceivedDataSize(USBH_HandleTypeDef *phost)
{
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef*) phost->pActiveClass->pData;

	if (phost->gState == HOST_CLASS)
	{
		return USBH_LL_GetLastXferSize(phost, handle->DataItf.InPipe);;
	}
	else
	{
		return 0;
	}
}

static USBH_StatusTypeDef PAX_D200_PrepareTransmit(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint32_t length)
{
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef*) phost->pActiveClass->pData;

	handle->pTxData = pbuff;
	handle->TxDataLength = length;
	handle->data_tx_state = PAX_D200_SEND_DATA;

	return USBH_OK;
}

static USBH_StatusTypeDef PAX_D200_PrepareReceive(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint32_t length)
{
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef*) phost->pActiveClass->pData;

	handle->pRxData = pbuff;
	handle->RxDataLength = length;
	handle->data_rx_state = PAX_D200_RECEIVE_DATA;

	return USBH_OK;
}

/**
* @brief  The function is responsible for sending data to the device
*  @param  pdev: Selected device
* @retval None
*/
static USBH_StatusTypeDef PAX_D200_ProcessTransmission(USBH_HandleTypeDef *phost)
{
	USBH_StatusTypeDef result = USBH_OK;
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef*) phost->pActiveClass->pData;

	switch (handle->data_tx_state)
	{

	case PAX_D200_SEND_DATA:
	{
		if (handle->TxDataLength > handle->DataItf.OutEpSize)
		{
			USBH_BulkSendData(phost, handle->pTxData, handle->DataItf.OutEpSize, handle->DataItf.OutPipe, 1);
		}
		else
		{
			USBH_BulkSendData(phost, handle->pTxData, handle->TxDataLength, handle->DataItf.OutPipe, 1);
		}

		handle->data_tx_state = PAX_D200_SEND_DATA_WAIT;
	}
	break;

	case PAX_D200_SEND_DATA_WAIT:
	{
		USBH_URBStateTypeDef URB_Status = USBH_LL_GetURBState(phost, handle->DataItf.OutPipe);

		/*Check the status done for transmission*/
		if (URB_Status == USBH_URB_DONE)
		{
			if (handle->TxDataLength > handle->DataItf.OutEpSize)
			{
				handle->TxDataLength -= handle->DataItf.OutEpSize;
				handle->pTxData += handle->DataItf.OutEpSize;
			}
			else
			{
				handle->TxDataLength = 0;
			}

			if (handle->TxDataLength > 0)
			{
				handle->data_tx_state = PAX_D200_SEND_DATA;
			}
			else
			{
				handle->data_tx_state = PAX_D200_IDLE_DATA_STATE;
//				USBH_PAX_D200_TransmitCallback(phost);
			}
		}
		else if (URB_Status == USBH_URB_NOTREADY)
			handle->data_tx_state = PAX_D200_SEND_DATA;
		else
			result = USBH_FAIL;
	}
	break;

	default:
	break;
	}

	return result;
}
/**
* @brief  This function responsible for reception of data from the device
*  @param  pdev: Selected device
* @retval None
*/

static USBH_StatusTypeDef PAX_D200_ProcessReception(USBH_HandleTypeDef *phost)
{
	USBH_StatusTypeDef result = USBH_OK;
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef*) phost->pActiveClass->pData;
	uint16_t length;

	switch (handle->data_rx_state)
	{
	case PAX_D200_RECEIVE_DATA:
	{
		result = CheckStatus(phost, USBH_BulkReceiveData(phost, handle->pRxData, handle->DataItf.InEpSize, handle->DataItf.InPipe));
		if (result == USBH_OK)
		{
			handle->data_rx_state = PAX_D200_RECEIVE_DATA_WAIT;
		}
		else
		{
			USBH_ErrLog("BulkReceiveData request error");
		}
	}
	break;

	case PAX_D200_RECEIVE_DATA_WAIT:
	{
		USBH_URBStateTypeDef URB_Status = USBH_LL_GetURBState(phost, handle->DataItf.InPipe);

		/*Check the status done for reception*/
		if (URB_Status == USBH_URB_DONE)
		{
			handle->realReceivedLen += handle->DataItf.InEpSize;

			length = USBH_LL_GetLastXferSize(phost, handle->DataItf.InPipe);

			if (((handle->RxDataLength - length) > 0) && (length > handle->DataItf.InEpSize))
			{
				handle->RxDataLength -= length;
				handle->pRxData += length;
				handle->data_rx_state = PAX_D200_RECEIVE_DATA;
			}
			else
			{
				handle->data_rx_state = PAX_D200_IDLE_DATA_STATE;
			}
		}
		else if (URB_Status == USBH_URB_IDLE)
		{
			// Ожидаем завершения операции чтения
		}
		else
		{
			USBH_DbgLog("Fail URB_Status: %d", URB_Status);
//			result = USBH_FAIL;
			result = USBH_INTERNAL_RESET;
			handle->data_rx_state = PAX_D200_IDLE_DATA_STATE;
		}
	}
	break;

	default:
	break;
	}

	return result;
}

#if 0
/**
* @brief  The function informs user that data have been received
*  @param  pdev: Selected device
* @retval None
*/
__weak void USBH_PAX_D200_TransmitCallback(USBH_HandleTypeDef *phost)
{
  
}
  
  /**
* @brief  The function informs user that data have been sent
*  @param  pdev: Selected device
* @retval None
*/
__weak void USBH_PAX_D200_ReceiveCallback(USBH_HandleTypeDef *phost, uint8_t *data, uint32_t len, uint32_t available)
{
  
}

__weak uint32_t USBH_ALL_TransmitRequest(uint8_t *writeData, const uint32_t maxLen)
{
	return 0;
}
#endif

static void PAX_D200_PrepareDataHeader(USBH_HandleTypeDef *phost, ST_BULK_IO *bioPack, uint8_t reqType, uint16_t len)
{
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef*) phost->pActiveClass->pData;

	handle->seqCount = (handle->seqCount + 1) & 0x0f;
	bioPack->seqNo = handle->seqCount;
	bioPack->reqType = reqType;
	bioPack->len = len;
}

static void PAX_D200_Reset(USBH_HandleTypeDef *phost)
{
	USBH_DbgLog("PAX_D200_Reset");

	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef *) phost->pActiveClass->pData;

	handle->state = PAX_D200_RESET_STATE;
	handle->stateProcessCommand = PAX_D200_IDLE_PROCESS_COMMAND_STATE;
	handle->seqCount = 0;
	handle->data_tx_state = PAX_D200_IDLE_DATA_STATE;
	handle->data_rx_state = PAX_D200_IDLE_DATA_STATE;
	handle->retries = 0;
	handle->retriesOfReset++;
}

static USBH_StatusTypeDef CheckStatus(USBH_HandleTypeDef *phost, USBH_StatusTypeDef result)
{
	PAX_D200_HandleTypeDef *handle = (PAX_D200_HandleTypeDef *) phost->pActiveClass->pData;
	if (result == USBH_OK)
		handle->retries = 0;
	else if (handle->retries++ >= MAX_RETRY_S)
	{
		PAX_D200_Reset(phost);
		if (handle->retriesOfReset > MAX_RETRY_S)
			handle->state = PAX_D200_FULL_RESET_STATE;
	}
	else
		HAL_Delay_us(100);

	return result;
}

//======================================================================================
// ======================== Из примера Linux драйвера, ttyPos ==========================

static int PAX_D200_VerifyChecksum(ST_BULK_IO *p_bio)
{
	unsigned char a, b;
	int i, dn;

	dn = p_bio->len + 4;
	a = 0;

	for (i = 2; i < dn; i++) {
		a ^= ((unsigned char *)p_bio)[i];
	}

	a ^= p_bio->seqNo & 0x0f;
	a ^= p_bio->reqType & 0x0f;
	b = (p_bio->seqNo & 0xf0) + (p_bio->reqType >> 4);
	if (a != b)
		return 1;

	/* clear checksum field */
	p_bio->seqNo &= 0x0f;
	p_bio->reqType &= 0x0f;
	return 0;
}

static void PAX_D200_SetChecksum(ST_BULK_IO *p_bio)
{
	unsigned char a;
	int i, dn;

	dn = p_bio->len + 4;
	a = 0;

	for (i = 2; i < dn; i++) {
		a ^= ((unsigned char *)p_bio)[i];
	}

	a ^= p_bio->seqNo & 0x0f;
	a ^= p_bio->reqType & 0x0f;

	/* fill high 4 bits of checksum into high 4 bits of ID field */
	p_bio->seqNo = (p_bio->seqNo & 0x0f) | (a & 0xf0);

	/* fill low 4 bits of checksum into high 4 bits of REQ_TYPE field */
	p_bio->reqType |= (a << 4);
}

static unsigned char PAX_D200_GetXOR(uint8_t *buf, unsigned int len)
{
	unsigned char a;
	unsigned int i;

	for(i=0,a=0;i<len;i++)
	  a^=buf[i];

	return a;
}
#endif
