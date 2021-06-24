#ifndef COMMON_GSM_COMMAND_H
#define COMMON_GSM_COMMAND_H

#include "utils/include/StringBuilder.h"

namespace Gsm {

#define SYNC_TIMEOUT 50
#define SYNC_TRIES_MAX 1000

#define POWER_BUTTON_PRESS	1200 //todo: разобратьс€ с таймаутами перезапуска модема
#define POWER_SHUTDOWN_CHECK 500
#define POWER_SHUTDOWN_TRY_NUMBER 10
#define POWER_SHUTDOWN_DELAY 1000

#define AT_COMMAND_MAX_SIZE 256 //todo: уточнить размер
#define AT_DATA_MAX_SIZE 256 //todo: уточнить размер
#define AT_RESPONSE_MAX_SIZE 512 //todo: уточнить размер
#define AT_COMMAND_DELAY 20
#define AT_COMMAND_TEXT_SIZE 64
#define AT_COMMAND_TIMEOUT 5000
#define AT_SOFT_RESET_MAX 3
#define AT_CREG_EVENT_TIMEOUT 50000
#define AT_CFUN_TIMEOUT 10000
#define AT_CIPSHUT_TIMEOUT 10000 //8.2.7 65000
#define AT_CGATT_TIMEOUT 40000 // 7.2.1 75 seconds
#define AT_CIICR_TIMEOUT 40000 // 8.2.10 85 seconds
#define AT_CIPSTART_TIMEOUT 160000 // 8.2.2 дл€ multiconn 75 секунд, но эмпирически проходит больше 100 секунд
#define AT_CIPSEND_TIMEOUT 20000
#define AT_CIPSEND_SEND_DELAY 50
#define AT_CIPRXGET_LEN_MAX 1460

#define GSM_RESTART_TIMEOUT 40000
#define GSM_TCP_IDLE_TIMEOUT 90000
#define GSM_TCP_TRY_MAX_NUMBER 3

enum Control {
	Control_Sync			= 0xB5,
	Control_SyncComplete	= 0x5B,
	Control_Erase			= 0x01,
	Control_Erasing			= 0x52, // 'R'(dec=82)
	Control_EraseError		= 0x45, // 'E'(dec=69)
	Control_EraseComplete	= 0x02,
	Control_Data			= 0x03,
	Control_DataConfirm		= 0x04,
	Control_DataEnd			= 0x05,
	Control_DataEndConfirm	= 0x06,
	Control_Reset			= 0x07,
	Control_ResetConfirm	= 0x08,
	Control_ENQ				= 0x15,
};

class Command {
public:
	enum Type {
		Type_Result = 0,
		Type_GsnData,
		Type_CgmrData,
		Type_CcidData,
		Type_CsqData,
		Type_CopsData,
		Type_CregData,
		Type_CgattData,
		Type_CifsrData,
		Type_CipPing,
		Type_CipStatus,
		Type_SendData,
		Type_RecvData,
		Type_CipClose,
	};
	enum Result {
		Result_OK = 0,
		Result_BUSY,
		Result_ERROR,
		Result_NO_CARRIER,
		Result_NO_ANSWER,
		Result_NO_DIAL_TONE,
		Result_TIMEOUT,
		Result_PARSER_ERROR,
		Result_LOGIC_ERROR,
		Result_RESET,        //  оманда не была выполнена. ѕроизошла перезагрузка GSM.
	};

	class Observer {
	public:
		virtual ~Observer() {}
		virtual void procResponse(Command::Result result, const char *data) = 0;
		virtual void procEvent(const char *data) = 0;
	};

	Command(Observer *observer);

	void set(Type type);
	void set(Type type, const char *text);
	void set(Type type, const char *text, uint32_t timeout);
	StringBuilder &setText();
	void setTimeout(uint32_t timeout);
	void setData(uint8_t *data, uint16_t dataLen);

	Type getType() const;
	const char *getText() const;
	uint32_t getTimeout() const;
	uint8_t *getData();
	uint16_t getDataLen() const;
	void deliverResponse(Result result, const char *data);

private:
	Type m_type;
	StringBuilder m_text;
	uint32_t m_timeout;
	uint8_t *m_data;
	uint16_t m_dataLen;
	Observer *m_observer;
};

class CommandProcessor {
public:
	virtual ~CommandProcessor() {}
	virtual bool execute(Command *command) = 0;
	virtual bool executeOutOfTurn(Command *command) = 0;
	virtual void reset() = 0;
	virtual void dump(StringBuilder *data) = 0;
};

class CommandParserInterface {
public:
	virtual ~CommandParserInterface() {}
	virtual void setObserver(Command::Observer *observer) = 0;
	virtual bool execute(Command *command) = 0;
	virtual void reset() = 0;
};

}

#endif
