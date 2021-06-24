#ifndef LIB_ERP_SYNCREQUEST_H_
#define LIB_ERP_SYNCREQUEST_H_

#include "common/config/include/ConfigModem.h"

class ErpSyncRequest {
public:
	ErpSyncRequest(ConfigModem *config);
	void make(uint32_t decimalPoint, uint16_t signalQuality, ConfigEventList *events, StringBuilder *reqData);
	uint32_t getSyncIndex() { return syncIndex; }

private:
	ConfigModem *config;
	uint32_t syncIndex;

	void makeParamNumber(uint16_t paramId, uint32_t paramValue, StringBuilder *reqData, bool first = false);
	void makeParamMoney(uint16_t paramId, uint32_t paramValue, StringBuilder *reqData, bool first = false);
	void makeParamString(uint16_t paramId, const char *paramValue, StringBuilder *reqData, bool first = false);
	void makeParamDateTime(uint16_t paramId, const DateTime *datetime, StringBuilder *reqData, bool first = false);
	void makeParamStat(uint16_t statId, StringBuilder *reqData, bool first = false);

	void makeDeviceList(StringBuilder *reqData);
	void makeDeviceModem(StringBuilder *reqData, bool &first);
	void makeDeviceAutomat(StringBuilder *reqData, bool &first);
	void makeDeviceFiscalRegistrar(StringBuilder *reqData, bool &first);
	void makeDeviceBillValidator(StringBuilder *reqData, bool &first);
	void makeDeviceCoinChanger(StringBuilder *reqData, bool &first);
	void makeDeviceCoinChangerCoinNominal(uint32_t coinNominal, StringBuilder *reqData, bool first = false);
	void makeDeviceCoinChangerTubeNominal(uint32_t coinNominal, uint32_t coinNumber, StringBuilder *reqData, bool first = false);
	void makeDeviceCashless(Mdb::DeviceContext *context, uint16_t deviceType, StringBuilder *reqData, bool &first);
	void makeDeviceEventList(ConfigErrorList *errors, StringBuilder *reqData);
	void makeDeviceEvent(ConfigEvent *error, StringBuilder *reqData);
	void makeEventList(uint32_t decimalPoint, ConfigEventList *events, StringBuilder *reqData);
	void makeEventSale(uint32_t decimalPoint, ConfigEvent *event, StringBuilder *reqData);
};

#endif
