#ifndef MDB_PROTOCOL_H_
#define MDB_PROTOCOL_H_

#include "config/include/ConfigErrorList.h"
#include "utils/include/NetworkProtocol.h"
#include "utils/include/DecimalPoint.h"
#include "event/include/Event2.h"

#include <stdint.h>

namespace Mdb {

#define MDB_PACKET_MAX_SIZE 40
#define MDB_DEVICE_MASK 0xF8
#define MDB_COMMAND_MASK 0x07
#define MDB_CONFIRM_SIZE 1
#define MDB_POLL_DATA_SIZE 30
#define MDB_MANUFACTURER_SIZE 3
#define MDB_MODEL_SIZE 12
#define MDB_SERIAL_NUMBER_SIZE 12
#define MDB_MANUFACTURER_CODE "EFR"
#define MDB_TRY_NUMBER 40
#define MDB_JUST_RESET_COUNT 100
#define MDB_POLL_TUBE_STATUS_COUNT 10
#define MDB_POLL_DIAGNOSTIC_COUNT 21
#define MDB_POLL_ENABLE_COUNT 32
#define MDB_REPEAT_COUNT 50
#define MDB_NON_RESPONSE_TIME 5000
#define MDB_ERROR_DATA_SIZE 32
#define MDB_SUBCOMMAND_NONE 0xFFFF

#define RUSSIAN_CURRENCY_RUB 1643 // валюта после деноминации 1998
#define RUSSIAN_CURRENCY_RUR 1810 // валюта до деноминации 1998

/**
 *  огда перифери€ отвечает блоком данных, VMC должен ответить ACK, NAK или RET.
 * ≈сли ћастер не может ответить в течение 5 мс тайм-аута (t-response), периферийное
 * устройство должно повторить блок данных или добавить его в следующий возможный
 * случай (то есть в более поздний POLL). “акое же поведение примен€етс€, когда
 * ћастер отвечает NAK.
 */
enum Control {
	Control_ACK	 = 0x00, // Acknowledgment/checksum correct
	Control_RET	 = 0xAA, // Retransmit the previously sent data. Only the VMC can transmit this byte
	Control_NAK	 = 0xFF, // Negative acknowledge
};

enum Timeout {
	Timeout_Reset	 = 200,
	Timeout_Poll	 = 200,
	Timeout_Recv	 = 50
};

enum Device {
	Device_CoinChanger		 = 0x08,
	Device_CashlessDevice1	 = 0x10,
	Device_ComGateway		 = 0x18,
	Device_BillValidator	 = 0x30,
	Device_CoinHopper1		 = 0x58,
	Device_CashlessDevice2	 = 0x60,
	Device_CoinHopper2		 = 0x70
};

enum FeatureLevel {
	FeatureLevel_1 = 0x01,
	FeatureLevel_2 = 0x02,
	FeatureLevel_3 = 0x03,
};

enum Mode {
	Mode_Normal    = 0x00,
	Mode_Expanded  = 0x01,
};

#pragma pack(push,1)
struct Header {
	uint8_t command;
	uint8_t subcommand;
};
#pragma pack(pop)

namespace BillValidator {

enum Route {
	Route_None		= 0x00,
	Route_Stacked	= 0x01,
	Route_Rejected	= 0x02,
};

}

namespace CoinChanger {

enum Route {
	Route_None		= 0x00,
	Route_Cashbox	= 0x01,
	Route_Tube		= 0x02,
	Route_Reject	= 0x03,
};

}

class EventDeposite : public EventInterface {
public:
	EventDeposite(uint16_t type) : EventInterface(type) {}
	EventDeposite(EventDeviceId deviceId) : EventInterface(deviceId, 0) {}
	EventDeposite(EventDeviceId deviceId, uint16_t type, uint8_t route, uint32_t nominal);
	void set(uint16_t type, uint8_t route, uint32_t nominal);
	uint8_t getRoute() { return route; }
	uint32_t getNominal() { return nominal; }
	virtual bool open(EventEnvelope *event);
	virtual bool pack(EventEnvelope *event);
private:
	uint8_t route;
	uint32_t nominal;
};

class EventError : public EventInterface {
public:
	EventError(uint16_t type);
	EventError(EventDeviceId deviceId, uint16_t type);
	uint16_t code;
	StringBuilder data;
	virtual bool open(EventEnvelope *event);
	virtual bool pack(EventEnvelope *event);
};

extern uint8_t calcCrc(const uint8_t *data, uint16_t len);
extern const char *deviceId2String(uint8_t deviceId);

class DeviceContext {
public:
	enum Device {
		Device_Modem			= 0x01,
		Device_Automat			= 0x02,
		Device_FiscalRegistrar	= 0x03,
		Device_BillValidator	= 0x04,
		Device_CoinChanger		= 0x05,
		Device_Cashless1		= 0x06,
	};

	enum Status {
		Status_NotFound	 = 0,
		Status_Init		 = 1,
		Status_Work		 = 2,
		Status_Error	 = 3,
		Status_Disabled	 = 4,
		Status_Enabled	 = 5,
	};

	enum Info {
		Info_Manufacturer				= 0x0001,
		Info_Model						= 0x0002,
		Info_SerialNumber				= 0x0003,
		Info_SoftwareVersion			= 0x0004,
		Info_FeatureLevel				= 0x0005,
		Info_DecimalPoint				= 0x0006,
		Info_ScalingFactor				= 0x0007,
		Info_Status						= 0x0008,
		Info_Error						= 0x0009,
		Info_Modem_SignalQuality		= 0x0101,
		Info_Modem_Iccid				= 0x0102,
		Info_Modem_PaymentBus			= 0x0103,
		Info_Modem_Temperature			= 0x0104,
		Info_Modem_Operator				= 0x0105,
		Info_BV_LastBillDate			= 0x0401,
		Info_BV_LastBillValue			= 0x0402,
		Info_BV_BillInStacker			= 0x0403,
		Info_BV_BillNominal				= 0x0404,
		Info_CC_LastCoinDate			= 0x0501,
		Info_CC_LastCoinValue			= 0x0502,
		Info_CC_TubeBalance				= 0x0503,
		Info_CC_TubeNominal				= 0x0504,
		Info_CC_CoinNominal				= 0x0505,
		Info_CL_LastPaymentDate			= 0x0601,
		Info_CL_LastPaymentValue		= 0x0602,
		Info_State						= 0x8000,
		Info_ResetCount					= 0x8001,
		Info_ProtocolErrorCount			= 0x8002,
		Info_Gsm_State					= 0x8101,
		Info_Gsm_HardResetCount			= 0x8102,
		Info_Gsm_SoftResetCount			= 0x8103,
		Info_Gsm_GprsResetCount			= 0x8104,
		Info_Gsm_CommandMax				= 0x8105,
		Info_Gsm_TcpConn0				= 0x8106,
		Info_Gsm_TcpConn1				= 0x8107,
		Info_Gsm_TcpConn2				= 0x8108,
		Info_Gsm_TcpConn3				= 0x8109,
		Info_Gsm_TcpConn4				= 0x810A,
		Info_Gsm_TcpConn5				= 0x810B,
		Info_Tcp_WrongStateCount		= 0x8110,
		Info_Tcp_ExecuteErrorCount		= 0x8120,
		Info_Tcp_CipStatusErrorCount	= 0x8130,
		Info_Tcp_GprsErrorCount			= 0x8140,
		Info_Tcp_CipSslErrorCount		= 0x8150,
		Info_Tcp_CipStartErrorCount		= 0x8160,
		Info_Tcp_ConnectErrorCount		= 0x8170,
		Info_Tcp_SendErrorCount			= 0x8180,
		Info_Tcp_RecvErrorCount			= 0x8190,
		Info_Tcp_UnwaitedCloseCount		= 0x81A0,
		Info_Tcp_IdleTimeoutCount		= 0x81B0,
		Info_Tcp_RxTxTimeoutCount		= 0x81C0,
		Info_Tcp_CipPing1ErrorCount		= 0x81D0,
		Info_Tcp_CipPing2ErrorCount		= 0x81E0,
		Info_Tcp_OtherErrorCount		= 0x81F0,
		Info_Erp_State					= 0x8201,
		Info_Erp_SyncErrorCount			= 0x8202,
		Info_Erp_SyncErrorMax			= 0x8203,
		Info_Eeprom_txTryMax			= 0x8301,
		Info_Eeprom_rxTryMax			= 0x8302,
		Info_VCC3						= 0x8401,
		Info_VCC5						= 0x8402,
		Info_VCC24						= 0x8403,
		Info_VCCBattery1				= 0x8404,
		Info_VCCBattery2				= 0x8405,
		Info_Temperature				= 0x8406,
		Info_I2C_TransmitTimeout		= 0x8501,
		Info_I2C_TransmitLastEvent		= 0x8502,
		Info_I2C_ReceiveTimeout			= 0x8503,
		Info_I2C_ReceiveLastEvent		= 0x8504,
		Info_I2C_WriteDataTimeout		= 0x8505,
		Info_I2C_WriteDataAckFailure	= 0x8506,
		Info_I2C_ReadDataTimeout		= 0x8507,
		Info_I2C_ReadDataLastEvent		= 0x8508,
		Info_I2C_Busy					= 0x8509,
		Info_I2C_WriteDataReinit		= 0x850A,
		Info_I2C_ReadDataReinit			= 0x850B,
		Info_I2C_ReinitLastEvent		= 0x850C,
		Info_MdbM_CrcErrorCount			= 0x8601,
		Info_MdbM_ResponseCount			= 0x8602,
		Info_MdbM_ConfirmCount			= 0x8603,
		Info_MdbM_TimeoutCount			= 0x8604,
		Info_MdbM_WrongState1			= 0x8605,
		Info_MdbM_WrongState2			= 0x8606,
		Info_MdbM_WrongState3			= 0x8607,
		Info_MdbM_WrongState4			= 0x8608,
		Info_MdbM_OtherError			= 0x8609,
		Info_ExeM_State					= 0x8701,
		Info_ExeM_CrcErrorCount			= 0x8702,
		Info_ExeM_ResponseCount			= 0x8703,
		Info_ExeM_TimeoutCount			= 0x8704,
		Info_MdbS_CrcErrorCount			= 0x8801,
		Info_MdbS_RequestCount			= 0x8802,
		Info_MdbS_BV_State				= 0x8901,
		Info_MdbS_BV_PollCount			= 0x8902,
		Info_MdbS_BV_DisableCount		= 0x8903,
		Info_MdbS_BV_EnableCount		= 0x8904,
		Info_MdbS_CC_State				= 0x8A01,
		Info_MdbS_CC_PollCount			= 0x8A02,
		Info_MdbS_CC_DisableCount		= 0x8A03,
		Info_MdbS_CC_EnableCount		= 0x8A04,
		Info_MdbS_CL_State				= 0x8B01,
		Info_MdbS_CL_PollCount			= 0x8B02,
		Info_MdbS_CL_DisableCount		= 0x8B03,
		Info_MdbS_CL_EnableCount		= 0x8B04,
	};

	DeviceContext(uint32_t masterDecimalPoint, RealTimeInterface *realtime);
	virtual ~DeviceContext() {}
	void setMasterDecimalPoint(uint32_t masterDecimalPoint);
	uint32_t getMasterDecimalPoint() const;
	void init(uint32_t deviceDecimalPoint, uint32_t scalingFactor);
	uint16_t getDecimalPoint() const;
	uint16_t getScalingFactor() const;
	virtual uint32_t value2money(uint32_t value);
	virtual uint32_t money2value(uint32_t value);

	void setStatus(Status status);
	Status getStatus() const;
	void setManufacturer(const uint8_t *str, uint16_t strLen);
	const char *getManufacturer() const;
	uint16_t getManufacturerSize() const;
	void setModel(const uint8_t *str, uint16_t strLen);
	const char *getModel() const;
	uint16_t getModelSize() const;
	void setSerialNumber(const uint8_t *str, uint16_t strLen);
	const char *getSerialNumber() const;
	uint16_t getSerialNumberSize() const;
	void setSoftwareVersion(uint16_t softwareVersion);
	uint16_t getSoftwareVersion() const;
	void setCurrency(uint16_t currency);
	uint16_t getCurrency() const;
	void setState(uint16_t state) { this->state = state; }
	uint16_t getState() { return state; }
	void incResetCount() { resetCount++; }
	uint32_t getResetCount() { return resetCount; }
	void incProtocolErrorCount() { protocolErrorCount++; }
	uint32_t getProtocolErrorCount() { return protocolErrorCount; }

	bool addError(uint16_t code, const char *str);
	void removeError(uint16_t code);
	void removeAll();
	ConfigErrorList *getErrors();

protected:
	Status status;
	StrParam<MDB_MANUFACTURER_SIZE> manufacturer;
	StrParam<MDB_MODEL_SIZE> model;
	StrParam<MDB_SERIAL_NUMBER_SIZE> serialNumber;
	uint16_t softwareVersion;
	DecimalPointConverter converter;
	uint16_t currency;
	uint16_t state;
	uint32_t resetCount;
	uint32_t protocolErrorCount;
	ConfigErrorList errors;
};

}

#endif
