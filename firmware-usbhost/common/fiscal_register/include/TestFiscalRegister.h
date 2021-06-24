#ifndef COMMON_FISCALREGISTER_TEST_H_
#define COMMON_FISCALREGISTER_TEST_H_

#include "FiscalRegister.h"

class TestFiscalRegister : public Fiscal::Register {
public:
	TestFiscalRegister(uint16_t deviceId, StringBuilder *str) : deviceId(deviceId), remoteFiscal(false), str(str), data(NULL) {}
	void setRemoteFiscal(bool remoteFiscal) { this->remoteFiscal = remoteFiscal; }

	virtual EventDeviceId getDeviceId() { return deviceId; }
	virtual bool isRemoteFiscal() { return remoteFiscal; }
	virtual void sale(Fiscal::Sale *data, uint32_t decimalPoint) {
		this->data = data;
//		*str << "<FR::sale(" << data->name.get() << "," << data->device.get() << "/" << data->paymentType << "," << data->price << "," << data->credit << "," << decimalPoint << ")>";
	}
	virtual void getLastSale() {}
	virtual void closeShift() {}

	void saleComplete(uint64_t fiscalRegister, uint64_t fiscalStorage, uint32_t fiscalDocument, uint32_t fiscalSign) {
		if(data == NULL) { return; }
		data->fiscalRegister = fiscalRegister;
		data->fiscalStorage = fiscalStorage;
		data->fiscalDocument = fiscalDocument;
		data->fiscalSign = fiscalSign;
		data = NULL;
	}
	void saleComplete() {
		if(data == NULL) { return; }
		data->fiscalRegister = 0;
		data->fiscalStorage = Fiscal::Status_Complete;
		data->fiscalDocument = 0;
		data->fiscalSign = 0;
		data = NULL;
	}
	void saleError() {
		if(data == NULL) { return; }
		data->fiscalRegister = 0;
		data->fiscalStorage = Fiscal::Status_Error;
		data->fiscalDocument = 0;
		data->fiscalSign = 0;
		data = NULL;
	}
	void saleInQueue() {
		if(data == NULL) { return; }
		data->fiscalRegister = 0;
		data->fiscalStorage = Fiscal::Status_InQueue;
		data->fiscalDocument = 0;
		data->fiscalSign = 0;
		data = NULL;
	}

private:
	EventDeviceId deviceId;
	bool remoteFiscal;
	StringBuilder *str;
	Fiscal::Sale *data;
};

#endif
