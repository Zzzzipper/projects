#include "config.h"
#ifdef DEVICE_USB2
/**
  ******************************************************************************
  * @file    usbh_PAX_D200.h
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file contains all the prototypes for the usbh_PAX_D200.c
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive  ----------------------------------------------*/
#ifndef __USBH_VENDOR_PAX_D200_H
#define __USBH_VENDOR_PAX_D200_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbh_core.h"
#include "usb2h_cdc.h"


/** @addtogroup USBH_LIB
* @{
*/

/** @addtogroup USBH_CLASS
* @{
*/

/** @addtogroup USBH_PAX_D200_CLASS
* @{
*/

/** @defgroup USBH_PAX_D200_CORE
* @brief This file is the Header file for usbh_core.c
* @{
*/ 


#define USB_VID_D200											0x1234
#define USB_PID_D200											0x0101


#define CS_INTERFACE                                            0x24
#define PAX_D200_PAGE_SIZE_64                                        0x40

/*Class-Specific Request Codes*/
#define PAX_D200_SEND_ENCAPSULATED_COMMAND                           0x00
#define PAX_D200_GET_ENCAPSULATED_RESPONSE                           0x01
#define PAX_D200_SET_COMM_FEATURE                                    0x02
#define PAX_D200_GET_COMM_FEATURE                                    0x03
#define PAX_D200_CLEAR_COMM_FEATURE                                  0x04

#define PAX_D200_SET_AUX_LINE_STATE                                  0x10
#define PAX_D200_SET_HOOK_STATE                                      0x11
#define PAX_D200_PULSE_SETUP                                         0x12
#define PAX_D200_SEND_PULSE                                          0x13
#define PAX_D200_SET_PULSE_TIME                                      0x14
#define PAX_D200_RING_AUX_JACK                                       0x15

#define PAX_D200_SET_LINE_CODING                                     0x20
#define PAX_D200_GET_LINE_CODING                                     0x21
#define PAX_D200_SET_CONTROL_LINE_STATE                              0x22
#define PAX_D200_SEND_BREAK                                          0x23

#define PAX_D200_SET_RINGER_PARMS                                    0x30
#define PAX_D200_GET_RINGER_PARMS                                    0x31
#define PAX_D200_SET_OPERATION_PARMS                                 0x32
#define PAX_D200_GET_OPERATION_PARMS                                 0x33
#define PAX_D200_SET_LINE_PARMS                                      0x34
#define PAX_D200_GET_LINE_PARMS                                      0x35
#define PAX_D200_DIAL_DIGITS                                         0x36
#define PAX_D200_SET_UNIT_PARAMETER                                  0x37
#define PAX_D200_GET_UNIT_PARAMETER                                  0x38
#define PAX_D200_CLEAR_UNIT_PARAMETER                                0x39
#define PAX_D200_GET_PROFILE                                         0x3A

#define PAX_D200_SET_ETHERNET_MULTICAST_FILTERS                      0x40
#define PAX_D200_SET_ETHERNET_POWER_MANAGEMENT_PATTERN FILTER        0x41
#define PAX_D200_GET_ETHERNET_POWER_MANAGEMENT_PATTERN FILTER        0x42
#define PAX_D200_SET_ETHERNET_PACKET_FILTER                          0x43
#define PAX_D200_GET_ETHERNET_STATISTIC                              0x44

#define PAX_D200_SET_ATM_DATA_FORMAT                                 0x50
#define PAX_D200_GET_ATM_DEVICE_STATISTICS                           0x51
#define PAX_D200_SET_ATM_DEFAULT_VC                                  0x52
#define PAX_D200_GET_ATM_VC_STATISTICS                               0x53


/* wValue for SetControlLineState*/
#define PAX_D200_ACTIVATE_CARRIER_SIGNAL_RTS                         0x0002
#define PAX_D200_DEACTIVATE_CARRIER_SIGNAL_RTS                       0x0000
#define PAX_D200_ACTIVATE_SIGNAL_DTR                                 0x0001
#define PAX_D200_DEACTIVATE_SIGNAL_DTR                               0x0000

 //======================================================================================
 // ======================== Из примера Linux драйвера, ttyPos.h ========================
 #if 1
 //#define POOL_SIZE		10241
 #define PAX_D200_MAX_DATA        508
 #else
 //#define POOL_SIZE		(120*1024+1)
 #define PAX_D200_MAX_DATA        65532
 #endif

 typedef struct
 {
 	uint8_t seqNo;
 	uint8_t reqType;		/* 0:OUT, 1:IN, 2:STAT, 3:RESET */
 	uint16_t len;
 	uint8_t data[PAX_D200_MAX_DATA];
 } __attribute__((packed)) ST_BULK_IO;

 typedef struct
 {
 	volatile uint32_t txBufSize;
 	volatile uint32_t rxBufSize;
 	volatile uint32_t txLeft;
 	volatile uint32_t rxLeft;
 } __attribute__((packed)) ST_BIO_STATE;

//#define IS_POOL_EMPTY(pool)	(pool.ReadPos == pool.WritePos)

 //======================================================================================

/* States for PAX_D200 State Machine */
typedef enum
{
  PAX_D200_IDLE_DATA_STATE = 0,
  PAX_D200_SEND_DATA,
  PAX_D200_SEND_DATA_WAIT,
  PAX_D200_RECEIVE_DATA,
  PAX_D200_RECEIVE_DATA_WAIT,
}
PAX_D200_DataStateTypeDef;

typedef enum
{
  PAX_D200_IDLE_STATE = 0,
  PAX_D200_RESET_STATE,
  PAX_D200_RESET_WAIT_STATE,
  PAX_D200_CLEAR_FEATURE,
  PAX_D200_SEND_CNTRL,
  PAX_D200_CHECK_MAX_DATA,
  PAX_D200_CHECK_MAX_DATA_WAIT,
  PAX_D200_CHECK_STATUS_COMMAND,
  PAX_D200_CHECK_STATUS_COMMAND_WAIT,
  PAX_D200_READ_DATA_COMMAND,
  PAX_D200_READ_DATA_COMMAND_WAIT,
  PAX_D200_WRITE_DATA_COMMAND,
  PAX_D200_WRITE_DATA_COMMAND_WAIT,

  PAX_D200_FULL_RESET_STATE,

}
PAX_D200_StateTypeDef;

typedef enum
{
  PAX_D200_IDLE_PROCESS_COMMAND_STATE = 0,
  PAX_D200_SEND_ONLY_REQUEST,
  PAX_D200_SEND_REQUEST,
  PAX_D200_RECEIVE_REQUEST,
  PAX_D200_RECEIVE_REQUEST_WAIT,
}
PAX_D200_StateProcessCommandTypeDef;

typedef struct _FunctionalDescriptorHeader				PAX_D200_HeaderFuncDesc_TypeDef;
typedef struct _CallMgmtFunctionalDescriptor			PAX_D200_CallMgmtFuncDesc_TypeDef;
typedef struct _AbstractCntrlMgmtFunctionalDescriptor	PAX_D200_AbstCntrlMgmtFuncDesc_TypeDef;
typedef struct _UnionFunctionalDescriptor				PAX_D200_UnionFuncDesc_TypeDef;

typedef struct _USBH_PAX_D200InterfaceDesc
{
  PAX_D200_HeaderFuncDesc_TypeDef           PAX_D200_HeaderFuncDesc;
  PAX_D200_CallMgmtFuncDesc_TypeDef         PAX_D200_CallMgmtFuncDesc;
  PAX_D200_AbstCntrlMgmtFuncDesc_TypeDef    PAX_D200_AbstCntrlMgmtFuncDesc;
  PAX_D200_UnionFuncDesc_TypeDef            PAX_D200_UnionFuncDesc;
}
PAX_D200_InterfaceDesc_Typedef;

typedef struct
{
  uint8_t              InPipe; 
  uint8_t              OutPipe;
  uint8_t              OutEp;
  uint8_t              InEp;
  uint8_t              buff[8];
  uint16_t             OutEpSize;
  uint16_t             InEpSize;  
}
PAX_D200_DataItfTypedef ;

/* Structure for PAX_D200 process */
typedef struct _PAX_D200_Process
{
  PAX_D200_DataItfTypedef               DataItf;
  uint8_t                           	*pTxData;
  uint8_t                           	*pRxData;
  uint32_t                           	TxDataLength;
  uint32_t                           	RxDataLength;
  PAX_D200_InterfaceDesc_Typedef        PAX_D200_Desc;
  PAX_D200_StateTypeDef                 state;
  PAX_D200_StateProcessCommandTypeDef	stateProcessCommand;
  PAX_D200_DataStateTypeDef             data_tx_state;
  PAX_D200_DataStateTypeDef             data_rx_state;
  uint8_t                          		Rx_Poll;

  uint8_t 								seqCount;
  uint16_t 								maxdata;
  uint32_t								rlen;
  uint32_t								wlen;
  ST_BIO_STATE							bioDevState;
  uint16_t								retries;
  uint16_t								retriesOfReset;
  uint32_t								realReceivedLen;
}
PAX_D200_HandleTypeDef;



extern USBH_ClassTypeDef  		Vendor_PAX_D200_Class;
#define USBH_PAX_D200_CLASS    &Vendor_PAX_D200_Class




uint16_t            USBH_PAX_D200_GetLastReceivedDataSize(USBH_HandleTypeDef *phost);

USBH_StatusTypeDef  USBH_PAX_D200_Stop(USBH_HandleTypeDef *phost);

void USBH_PAX_D200_TransmitCallback(USBH_HandleTypeDef *phost);

void USBH_PAX_D200_ReceiveCallback(USBH_HandleTypeDef *phost, uint8_t *data, uint32_t len, uint32_t available);

uint32_t USBH_ALL_TransmitRequest(uint8_t *writeData, const uint32_t maxLen);


#ifdef __cplusplus
}
#endif

#endif
#endif
