#include "MdbMasterCashlessStack.h"

void MdbMasterCashlessStack::push(MdbMasterCashlessInterface *masterCashless) {
	if(masterCashless != NULL) {
		list.add(masterCashless);
	}
}

void MdbMasterCashlessStack::reset() {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		list.get(i)->reset();
	}
}

void MdbMasterCashlessStack::enable() {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		list.get(i)->enable();
	}
}

void MdbMasterCashlessStack::disable() {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		list.get(i)->disable();
	}
}

void MdbMasterCashlessStack::disableNotRefundAble() {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		MdbMasterCashlessInterface *masterCashless = list.get(i);
		if(masterCashless->isRefundAble() == false) {
			masterCashless->disable();
		}
	}
}

bool MdbMasterCashlessStack::sale(uint16_t productId, uint32_t productPrice, const char *productName, uint32_t wareId) {
	uint16_t saleNum = 0;
	for(uint16_t i = 0; i < list.getSize(); i++) {
		if(list.get(i)->sale(productId, productPrice, productName, wareId) == true) {
			saleNum++;
		}
	}
	return (saleNum > 0);
}

void MdbMasterCashlessStack::closeSession() {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		list.get(i)->closeSession();
	}
}

MdbMasterCashlessInterface *MdbMasterCashlessStack::find(uint16_t deviceId) {
	for(uint16_t i = 0; i < list.getSize(); i++) {
		MdbMasterCashlessInterface *masterCashless = list.get(i);
		if(masterCashless->getDeviceId().getValue() == deviceId) {
			return masterCashless;
		}
	}
	return NULL;
}
