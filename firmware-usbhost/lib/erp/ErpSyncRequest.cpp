#include "ErpSyncRequest.h"

#include "common/utils/include/CodePage.h"
#include "common/utils/include/Version.h"
#include "common/logger/include/Logger.h"

#include "lib/adc/Adc.h"

enum AutomatDeviceType {
	AutomatDeviceType_Modem 			= 0x01,
	AutomatDeviceType_Automat			= 0x02,
	AutomatDeviceType_FiscalRegistrar	= 0x03,
	AutomatDeviceType_MdbBillValidator	= 0x04,
	AutomatDeviceType_MdbCoinChanger	= 0x05,
	AutomatDeviceType_MdbCashless1		= 0x06,
	AutomatDeviceType_MdbCashless2		= 0x07,
	AutomatDeviceType_Ext1Cashless		= 0x08,
	AutomatDeviceType_Screen			= 0x09,
	AutomatDeviceType_Nfc				= 0x0A,
	AutomatDeviceType_Usb1Cashless		= 0x0B,
};

ErpSyncRequest::ErpSyncRequest(ConfigModem *config) :
	config(config)
{

}

void ErpSyncRequest::make(uint32_t decimalPoint, uint16_t signalQuality, ConfigEventList *events, StringBuilder *reqData) {
	reqData->clear();
	*reqData << "{";
	*reqData << "\"hv\":\"" << LOG_VERSION(config->getBoot()->getHardwareVersion()) << "\",";
	*reqData << "\"sv\":\"" << LOG_VERSION(config->getBoot()->getFirmwareVersion()) << "\",";
	*reqData << "\"release\":" << config->getBoot()->getFirmwareRelease() << ",";
	*reqData << "\"sq\":" << signalQuality << ",";
	*reqData << "\"devices\":[";
	makeDeviceList(reqData);
	*reqData << "],\"events\":[";
	makeEventList(decimalPoint, events, reqData);
	*reqData << "]}";
}

void ErpSyncRequest::makeParamNumber(uint16_t paramId, uint32_t paramValue, StringBuilder *reqData, bool first) {
	if(first == false) { *reqData << ","; }
	*reqData << "{\"t\":" << paramId << ",\"v\":" << paramValue << "}";
}

void ErpSyncRequest::makeParamMoney(uint16_t paramId, uint32_t paramValue, StringBuilder *reqData, bool first) {
	if(first == false) { *reqData << ","; }
	*reqData << "{\"t\":" << paramId << ",\"v\":" << convertDecimalPoint(config->getAutomat()->getDecimalPoint(), 2, paramValue) << "}";
}

void ErpSyncRequest::makeParamString(uint16_t paramId, const char *paramValue, StringBuilder *reqData, bool first) {
	if(first == false) { *reqData << ","; }
	*reqData << "{\"t\":" << paramId << ",\"v\":\""; convertWin1251ToJsonUnicode(paramValue, reqData); *reqData << "\"}";
}

void ErpSyncRequest::makeParamDateTime(uint16_t paramId, const DateTime *datetime, StringBuilder *reqData, bool first) {
	if(first == false) { *reqData << ","; }
	*reqData << "{\"t\":" << paramId << ",\"v\":\""; datetime2string(datetime, reqData); *reqData << "\"}";
}

void ErpSyncRequest::makeParamStat(uint16_t statId, StringBuilder *reqData, bool first) {
	StatNode *node = config->getStat()->get(statId);
	if(node == NULL) { return; }
	makeParamNumber(statId, node->get(), reqData, first);
}

void ErpSyncRequest::makeDeviceList(StringBuilder *reqData) {
	bool first = true;
	makeDeviceModem(reqData, first);
	makeDeviceAutomat(reqData, first);
	makeDeviceBillValidator(reqData, first);
	makeDeviceCoinChanger(reqData, first);
	makeDeviceCashless(config->getAutomat()->getMdb1CashlessContext(), AutomatDeviceType_MdbCashless1, reqData, first);
	makeDeviceCashless(config->getAutomat()->getMdb2CashlessContext(), AutomatDeviceType_MdbCashless2, reqData, first);
	makeDeviceCashless(config->getAutomat()->getExt1CashlessContext(), AutomatDeviceType_Ext1Cashless, reqData, first);
	makeDeviceCashless(config->getAutomat()->getUsb1CashlessContext(), AutomatDeviceType_Usb1Cashless, reqData, first);
	makeDeviceCashless(config->getAutomat()->getScreenContext(), AutomatDeviceType_Screen, reqData, first);
	makeDeviceCashless(config->getAutomat()->getNfcContext(), AutomatDeviceType_Nfc, reqData, first);
	makeDeviceFiscalRegistrar(reqData, first);
}

void ErpSyncRequest::makeDeviceModem(StringBuilder *reqData, bool &first) {
	if(first == false) { *reqData << ","; }
	Adc *adc = Adc::get();
	*reqData << "{\"type\":1,\"state\":" << Mdb::DeviceContext::Status_Work << ",\"params\":[";
	makeParamString(Mdb::DeviceContext::Info_Modem_Iccid, config->getBoot()->getIccid(), reqData, true);
	makeParamString(Mdb::DeviceContext::Info_Modem_Operator, config->getBoot()->getOper(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_Modem_PaymentBus, config->getAutomat()->getPaymentBus(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_Modem_Temperature, adc->read(Adc::TEMP_SENSOR), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_VCC3, adc->read(Adc::VCC_3), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_VCC5, adc->read(Adc::VCC_5), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_VCC24, adc->read(Adc::VCC_24), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_VCCBattery1, adc->read(Adc::VCC_BAT1), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_VCCBattery2, adc->read(Adc::VCC_BAT2), reqData);
	makeParamStat(Mdb::DeviceContext::Info_Gsm_State, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Gsm_HardResetCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Gsm_SoftResetCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Gsm_CommandMax, reqData);
#ifdef DEBUG_PROTOCOL
	makeParamStat(Mdb::DeviceContext::Info_Gsm_TcpConn0, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Gsm_TcpConn1, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Gsm_TcpConn2, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Gsm_TcpConn3, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Gsm_TcpConn4, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Gsm_TcpConn5, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_WrongStateCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_ExecuteErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_CipStatusErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_GprsErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_CipSslErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_CipStartErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_ConnectErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_SendErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_RecvErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_UnwaitedCloseCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_IdleTimeoutCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_RxTxTimeoutCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_CipPing1ErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_CipPing2ErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Tcp_OtherErrorCount, reqData);
#endif
	makeParamStat(Mdb::DeviceContext::Info_Erp_State, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Erp_SyncErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Erp_SyncErrorMax, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Eeprom_txTryMax, reqData);
	makeParamStat(Mdb::DeviceContext::Info_Eeprom_rxTryMax, reqData);
	makeParamStat(Mdb::DeviceContext::Info_I2C_TransmitTimeout, reqData);
	makeParamStat(Mdb::DeviceContext::Info_I2C_TransmitLastEvent, reqData);
	makeParamStat(Mdb::DeviceContext::Info_I2C_ReceiveTimeout, reqData);
	makeParamStat(Mdb::DeviceContext::Info_I2C_ReceiveLastEvent, reqData);
	makeParamStat(Mdb::DeviceContext::Info_I2C_WriteDataTimeout, reqData);
	makeParamStat(Mdb::DeviceContext::Info_I2C_WriteDataAckFailure, reqData);
	makeParamStat(Mdb::DeviceContext::Info_I2C_ReadDataTimeout, reqData);
	makeParamStat(Mdb::DeviceContext::Info_I2C_ReadDataLastEvent, reqData);
	makeParamStat(Mdb::DeviceContext::Info_I2C_Busy, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbM_ResponseCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbM_ConfirmCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbM_CrcErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbM_TimeoutCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbM_WrongState1, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbM_WrongState2, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbM_WrongState3, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbM_WrongState4, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbM_OtherError, reqData);
	makeParamStat(Mdb::DeviceContext::Info_ExeM_State, reqData);
	makeParamStat(Mdb::DeviceContext::Info_ExeM_CrcErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_ExeM_ResponseCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_ExeM_TimeoutCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_CrcErrorCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_RequestCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_BV_State, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_BV_PollCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_BV_DisableCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_BV_EnableCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_CC_State, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_CC_PollCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_CC_DisableCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_CC_EnableCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_CL_State, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_CL_PollCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_CL_DisableCount, reqData);
	makeParamStat(Mdb::DeviceContext::Info_MdbS_CL_EnableCount, reqData);
	*reqData << "],\"events\":[]}";
	first = false;
}

void ErpSyncRequest::makeDeviceAutomat(StringBuilder *reqData, bool &first) {
	if(first == false) { *reqData << ","; }
	Mdb::DeviceContext *context = config->getAutomat()->getSMContext();
	Mdb::DeviceContext::Status status = context->getStatus();
	if(context->getErrors()->getSize() > 0) {
		status = Mdb::DeviceContext::Status_Error;
	}
	*reqData << "{\"type\":2,\"state\":" << status << ",\"params\":[";
	makeParamString(Mdb::DeviceContext::Info_Manufacturer, context->getManufacturer(), reqData, true);
	makeParamString(Mdb::DeviceContext::Info_Model, context->getModel(), reqData);
	makeParamString(Mdb::DeviceContext::Info_SerialNumber, context->getSerialNumber(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_SoftwareVersion, context->getSoftwareVersion(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_State, context->getState(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ProtocolErrorCount, context->getProtocolErrorCount(), reqData);
	*reqData << "],\"events\":[";
	makeDeviceEventList(context->getErrors(), reqData);
	*reqData << "]}";
	first = false;
}

void ErpSyncRequest::makeDeviceBillValidator(StringBuilder *reqData, bool &first) {
	MdbBillValidatorContext *context = config->getAutomat()->getBVContext();
	if(first == false) { *reqData << ","; }
	*reqData << "{\"type\":4,\"state\":" << context->getStatus() << ",\"params\":[";
	makeParamString(Mdb::DeviceContext::Info_Manufacturer, context->getManufacturer(), reqData, true);
	makeParamString(Mdb::DeviceContext::Info_Model, context->getModel(), reqData);
	makeParamString(Mdb::DeviceContext::Info_SerialNumber, context->getSerialNumber(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_SoftwareVersion, context->getSoftwareVersion(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_DecimalPoint, context->getDecimalPoint(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ScalingFactor, context->getScalingFactor(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_State, context->getState(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ResetCount, context->getResetCount(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ProtocolErrorCount, context->getProtocolErrorCount(), reqData);
	if(context->getLastBillValue() != 0) {
		makeParamDateTime(Mdb::DeviceContext::Info_BV_LastBillDate, context->getLastBillDatetime(), reqData);
		makeParamMoney(Mdb::DeviceContext::Info_BV_LastBillValue, context->getLastBillValue(), reqData);
	}
	if(context->getBillInStacker() != BV_STACKER_UNKNOWN) {
		makeParamNumber(Mdb::DeviceContext::Info_BV_BillInStacker, context->getBillInStacker(), reqData);
	}
	for(uint16_t i = 0; i < 16; i++) {
		uint32_t billNominal = context->getBillNominal(i);
		if(billNominal > 0) { makeParamMoney(Mdb::DeviceContext::Info_BV_BillNominal, billNominal, reqData); }
	}
	*reqData << "],\"events\":[";
	makeDeviceEventList(context->getErrors(), reqData);
	*reqData << "]}";
	first = false;
}

void ErpSyncRequest::makeDeviceCoinChanger(StringBuilder *reqData, bool &first) {
	MdbCoinChangerContext *context = config->getAutomat()->getCCContext();
	if(first == false) { *reqData << ","; }
	*reqData << "{\"type\":5,\"state\":" << context->getStatus() << ",\"params\":[";
	makeParamString(Mdb::DeviceContext::Info_Manufacturer, context->getManufacturer(), reqData, true);
	makeParamString(Mdb::DeviceContext::Info_Model, context->getModel(), reqData);
	makeParamString(Mdb::DeviceContext::Info_SerialNumber, context->getSerialNumber(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_SoftwareVersion, context->getSoftwareVersion(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_DecimalPoint, context->getDecimalPoint(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ScalingFactor, context->getScalingFactor(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_State, context->getState(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ResetCount, context->getResetCount(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ProtocolErrorCount, context->getProtocolErrorCount(), reqData);
	if(context->getLastCoinValue() != 0) {
		makeParamDateTime(Mdb::DeviceContext::Info_CC_LastCoinDate, context->getLastCoinDatetime(), reqData);
		makeParamMoney(Mdb::DeviceContext::Info_CC_LastCoinValue, context->getLastCoinValue(), reqData);
	}
	makeParamMoney(Mdb::DeviceContext::Info_CC_TubeBalance, context->getInTubeValue(), reqData);
	for(uint16_t i = 0; i < 16; i++) {
		MdbCoin *coin = context->get(i);
		if(coin == NULL) { continue; }
		if(coin->getNominal() > 0) { makeDeviceCoinChangerCoinNominal(coin->getNominal(), reqData); }
		if(coin->getInTube() == true) { makeDeviceCoinChangerTubeNominal(coin->getNominal(), coin->getNumber(), reqData); }
	}
	*reqData << "],\"events\":[";
	makeDeviceEventList(context->getErrors(), reqData);
	*reqData << "]}";
	first = false;
}

void ErpSyncRequest::makeDeviceCoinChangerCoinNominal(uint32_t coinNominal, StringBuilder *reqData, bool first) {
	if(first == false) { *reqData << ","; }
	uint32_t nominal = coinNominal < MDB_CC_TOKEN ? convertDecimalPoint(config->getAutomat()->getDecimalPoint(), 2, coinNominal) : MDB_CC_TOKEN;
	*reqData << "{\"t\":" << Mdb::DeviceContext::Info_CC_CoinNominal << ",\"v\":\"" << nominal << "\"}";
}

void ErpSyncRequest::makeDeviceCoinChangerTubeNominal(uint32_t coinNominal, uint32_t coinNumber, StringBuilder *reqData, bool first) {
	if(first == false) { *reqData << ","; }
	uint32_t nominal = coinNominal < MDB_CC_TOKEN ? convertDecimalPoint(config->getAutomat()->getDecimalPoint(), 2, coinNominal) : MDB_CC_TOKEN;
	*reqData << "{\"t\":" << Mdb::DeviceContext::Info_CC_TubeNominal << ",\"v\":\"" << nominal << "*" << coinNumber << "\"}";
}

void ErpSyncRequest::makeDeviceCashless(Mdb::DeviceContext *context, uint16_t deviceType, StringBuilder *reqData, bool &first) {
	if(first == false) { *reqData << ","; }
	*reqData << "{\"type\":" << deviceType << ",\"state\":" << context->getStatus() << ",\"params\":[";
	makeParamString(Mdb::DeviceContext::Info_Manufacturer, context->getManufacturer(), reqData, true);
	makeParamString(Mdb::DeviceContext::Info_Model, context->getModel(), reqData);
	if(context->getSerialNumber()[0] != '\0') { makeParamString(Mdb::DeviceContext::Info_SerialNumber, context->getSerialNumber(), reqData); }
	makeParamNumber(Mdb::DeviceContext::Info_SoftwareVersion, context->getSoftwareVersion(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_DecimalPoint, context->getDecimalPoint(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ScalingFactor, context->getScalingFactor(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_State, context->getState(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ResetCount, context->getResetCount(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ProtocolErrorCount, context->getProtocolErrorCount(), reqData);
	*reqData << "],\"events\":[";
	makeDeviceEventList(context->getErrors(), reqData);
	*reqData << "]}";
	first = false;
}

void ErpSyncRequest::makeDeviceFiscalRegistrar(StringBuilder *reqData, bool &first) {
	Fiscal::Context *context = config->getAutomat()->getFiscalContext();
	if(first == false) { *reqData << ","; }
	*reqData << "{\"type\":3,\"state\":" << context->getStatus() << ",\"params\":[";
	makeParamString(Mdb::DeviceContext::Info_Manufacturer, context->getManufacturer(), reqData, true);
	makeParamString(Mdb::DeviceContext::Info_Model, context->getModel(), reqData);
	if(context->getSerialNumber()[0] != '\0') { makeParamString(Mdb::DeviceContext::Info_SerialNumber, context->getSerialNumber(), reqData); }
	makeParamNumber(Mdb::DeviceContext::Info_State, context->getState(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ResetCount, context->getResetCount(), reqData);
	makeParamNumber(Mdb::DeviceContext::Info_ProtocolErrorCount, context->getProtocolErrorCount(), reqData);
	*reqData << "],\"events\":[";
	makeDeviceEventList(context->getErrors(), reqData);
	*reqData << "]}";
	first = false;
}

void ErpSyncRequest::makeDeviceEventList(ConfigErrorList *errors, StringBuilder *reqData) {
	bool first = true;
	for(uint16_t i = 0; i < errors->getSize(); i++) {
		if(first == true) { first = false; } else { *reqData << ","; }
		ConfigEvent *error = errors->get(i);
		makeDeviceEvent(error, reqData);
	}
}

void ErpSyncRequest::makeDeviceEvent(ConfigEvent *error, StringBuilder *reqData) {
	DateTime *datetime = error->getDate();
	*reqData << "{\"t\":" << error->getCode();
	*reqData << ",\"d\":\""; datetime2string(datetime, reqData); *reqData << "\"";
	*reqData << ",\"v\":\""; convertWin1251ToJsonUnicode(error->getString(), reqData); *reqData << "\"";
	*reqData << "}";
}

void ErpSyncRequest::makeEventList(uint32_t decimalPoint, ConfigEventList *events, StringBuilder *reqData) {
	ConfigEventIterator iterator(events);
	if(iterator.unsync() == false) {
		LOG_DEBUG(LOG_MODEM, "No unsynced events");
		syncIndex = CONFIG_EVENT_UNSET;
		return;
	}

	bool first = true;
	for(uint16_t count = 0; count < 20; count++) {
		syncIndex = iterator.getIndex();
		if(iterator.hasLoadError() == true) {
			continue;
		}
		if(first == true) { first = false; } else { *reqData << ","; }
		makeEventSale(decimalPoint, iterator.getEvent(), reqData);
		if(iterator.next() == false) {
			break;
		}
	}
}

void ErpSyncRequest::makeEventSale(uint32_t decimalPoint, ConfigEvent *event, StringBuilder *reqData) {
	DateTime *datetime = event->getDate();
	*reqData << "{\"date\":\""; datetime2string(datetime, reqData); *reqData << "\"";
	*reqData << ",\"type\":" << event->getCode();
	if(event->getCode() == ConfigEvent::Type_Sale) {
		ConfigEventSale *sale = event->getSale();
		*reqData << ",\"select_id\":\"" << sale->selectId.get() << "\"";
		if(sale->wareId > 0) { *reqData << ",\"ware_id\":" << sale->wareId; }
		*reqData << ",\"name\":\""; convertWin1251ToJsonUnicode(sale->name.get(), reqData); *reqData << "\"";
		*reqData << ",\"payment_device\":\"" << sale->device.get() <<  "\"";
		*reqData << ",\"price_list\":" << sale->priceList;
		*reqData << ",\"value\":" << convertDecimalPoint(decimalPoint, 2, sale->price);
		*reqData << ",\"tax_system\":" << sale->taxSystem;
		*reqData << ",\"tax_rate\":" << sale->taxRate;
		*reqData << ",\"tax_value\":" << sale->taxValue;
#ifdef NEW_CONFIG
		*reqData << ",\"loyality_type\":" << sale->loyalityType;
		*reqData << ",\"loyality_code\":\""; convertBinToBase64(sale->loyalityCode.getData(), sale->loyalityCode.getLen(), reqData); *reqData << "\"";
#endif
		if(sale->fiscalStorage > 0) { *reqData << ",\"fn\":" << sale->fiscalStorage; }
		if(sale->fiscalDocument > 0) { *reqData << ",\"fd\":" << sale->fiscalDocument; }
		if(sale->fiscalSign > 0) { *reqData << ",\"fp\":" << sale->fiscalSign; }
	} else {
		*reqData << ",\"name\":\""; convertWin1251ToJsonUnicode(event->getString(), reqData); *reqData << "\"";
	}
	*reqData << "}";
}
