#ifndef COMMON_UTILS_DECIMALPOINT_H_
#define COMMON_UTILS_DECIMALPOINT_H_

#include <stdint.h>

#define DECIMAL_POINT_MIN 0
#define DECIMAL_POINT_MAX 3
#define DECIMAL_POINT_DEFAULT 2
#define SCREEN_DECIMAL_POINT 2
#define SCALING_FACTOR_MIN 1
#define SCALING_FACTOR_MAX 65535
#define SCALING_FACTOR_DEFAULT 1

class DecimalPointConverter {
public:
	DecimalPointConverter(uint32_t masterDecimalPoint);
	bool setMasterDecimalPoint(uint32_t masterDecimalPoint);
	bool setDeviceDecimalPoint(uint32_t deviceDecimalPoint);
	bool setScalingFactor(uint32_t scalingFactor);
	uint32_t getMasterDecimalPoint() const;
	uint32_t getDeviceDecimalPoint() const;
	uint32_t getScalingFactor() const;
	uint32_t convertMasterToDevice(uint32_t masterValue) const;
	uint32_t convertDeviceToMaster(uint32_t deviceValue) const;

private:
	uint32_t masterDecimalPoint;
	uint32_t deviceDecimalPoint;
	uint32_t devider;
	uint32_t scalingFactor;

	uint32_t calcDevider(uint16_t decimalPoint);
};

extern uint32_t convertDecimalPoint(uint32_t from, uint32_t to, uint32_t value);

#endif
