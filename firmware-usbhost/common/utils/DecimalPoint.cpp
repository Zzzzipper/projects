#include "include/DecimalPoint.h"
#include "logger/include/Logger.h"

DecimalPointConverter::DecimalPointConverter(uint32_t masterDecimalPoint) :
	masterDecimalPoint(masterDecimalPoint),
	deviceDecimalPoint(masterDecimalPoint),
	devider(1),
	scalingFactor(SCALING_FACTOR_DEFAULT)
{

}

bool DecimalPointConverter::setMasterDecimalPoint(uint32_t masterDecimalPoint) {
	if(masterDecimalPoint > DECIMAL_POINT_MAX) {
		LOG_ERROR(LOG_CFG, "Wrong decimal point value " << masterDecimalPoint);
		return false;
	}

	this->masterDecimalPoint = masterDecimalPoint;
	if(masterDecimalPoint > deviceDecimalPoint) {
		devider = calcDevider(masterDecimalPoint - deviceDecimalPoint);
		return true;
	} else {
		devider = calcDevider(deviceDecimalPoint - masterDecimalPoint);
		return true;
	}
}

bool DecimalPointConverter::setDeviceDecimalPoint(uint32_t deviceDecimalPoint) {
	if(deviceDecimalPoint > DECIMAL_POINT_MAX) {
		LOG_ERROR(LOG_CFG, "Wrong decimal point value " << deviceDecimalPoint);
		return false;
	}

	this->deviceDecimalPoint = deviceDecimalPoint;
	if(masterDecimalPoint > deviceDecimalPoint) {
		devider = calcDevider(masterDecimalPoint - deviceDecimalPoint);
		return true;
	} else {
		devider = calcDevider(deviceDecimalPoint - masterDecimalPoint);
		return true;
	}
}

bool DecimalPointConverter::setScalingFactor(uint32_t scalingFactor) {
	if(scalingFactor < SCALING_FACTOR_MIN || scalingFactor > SCALING_FACTOR_MAX) {
		LOG_ERROR(LOG_CFG, "Wrong scaling factor" << scalingFactor);
		return false;
	}
	this->scalingFactor = scalingFactor;
	return true;
}

uint32_t DecimalPointConverter::getMasterDecimalPoint() const {
	return masterDecimalPoint;
}

uint32_t DecimalPointConverter::getDeviceDecimalPoint() const {
	return deviceDecimalPoint;
}

uint32_t DecimalPointConverter::getScalingFactor() const {
	return scalingFactor;
}

uint32_t DecimalPointConverter::convertMasterToDevice(uint32_t masterValue) const {
	if(masterDecimalPoint == deviceDecimalPoint) {
		return masterValue / scalingFactor;
	} else if(masterDecimalPoint > deviceDecimalPoint) {
		return masterValue / devider / scalingFactor;
	} else {
		return masterValue * devider / scalingFactor;
	}
}

uint32_t DecimalPointConverter::convertDeviceToMaster(uint32_t deviceValue) const {
	if(masterDecimalPoint == deviceDecimalPoint) {
		return deviceValue * scalingFactor;
	} else if(masterDecimalPoint > deviceDecimalPoint) {
		return deviceValue * scalingFactor * devider;
	} else {
		return deviceValue * scalingFactor / devider;
	}
}

uint32_t DecimalPointConverter::calcDevider(uint16_t decimalPoint) {
	uint32_t d = 1;
	for(uint16_t i = 0; i < decimalPoint; i++) {
		d = d * 10;
	}
	return d;
}

uint32_t convertDecimalPoint(uint32_t from, uint32_t to, uint32_t value) {
	if(from > to) {
		uint32_t dif = from - to;
		switch(dif) {
		case 2: return (value/100);
		case 1: return (value/10);
		default: return value;
		}
	} else {
		uint32_t dif = to - from;
		switch(dif) {
		case 2: return (value*100);
		case 1: return (value*10);
		default: return value;
		}
	}
	return 0;
}
