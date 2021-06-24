#pragma once

/*
 * Пример взят из файла stm32l1xx_ll_adc.h библиотеки HAL.
 * Для stm32f407 почему-то такой файл отсутствовал. Сделал по аналогии.
 *
 */


/* ADC internal channels related definitions */
/* Internal voltage reference VrefInt */
#define VREFINT_CAL_ADDR                   ((uint16_t*) (0x1FFF7A2AU)) /* Internal voltage reference, address of parameter VREFINT_CAL: VrefInt ADC raw data acquired at temperature 30 DegC (tolerance: +-5 DegC), Vref+ = 3.0 V (tolerance: +-10 mV). */
#define VREFINT_CAL_VREF                   ( 3300U)                    /* Analog voltage reference (Vref+) value with which temperature sensor has been calibrated in production (tolerance: +-10 mV) (unit: mV). */
/* Temperature sensor */
#define TEMPSENSOR_CAL1_ADDR               ((uint16_t*) (0x1FFF7A2CU)) /* Internal temperature sensor, address of parameter TS_CAL1: On STM32L1, temperature sensor ADC raw data acquired at temperature  30 DegC (tolerance: +-5 DegC), Vref+ = 3.0 V (tolerance: +-10 mV). */
#define TEMPSENSOR_CAL2_ADDR               ((uint16_t*) (0x1FFF7A2EU)) /* Internal temperature sensor, address of parameter TS_CAL2: On STM32L1, temperature sensor ADC raw data acquired at temperature 110 DegC (tolerance: +-5 DegC), Vref+ = 3.0 V (tolerance: +-10 mV). */
#define TEMPSENSOR_CAL1_TEMP               (( int32_t)   30)           /* Internal temperature sensor, temperature at which temperature sensor has been calibrated in production for data into TEMPSENSOR_CAL1_ADDR (tolerance: +-5 DegC) (unit: DegC). */
#define TEMPSENSOR_CAL2_TEMP               (( int32_t)  110)           /* Internal temperature sensor, temperature at which temperature sensor has been calibrated in production for data into TEMPSENSOR_CAL2_ADDR (tolerance: +-5 DegC) (unit: DegC). */
#define TEMPSENSOR_CAL_VREFANALOG          ( 3300U)                    /* Analog voltage reference (Vref+) voltage with which temperature sensor has been calibrated in production (+-10 mV) (unit: mV). */



/* ADC registers bits positions */
#define ADC_CR1_RES_BITOFFSET_POS          (24U) /* Value equivalent to POSITION_VAL(ADC_CR1_RES) */
#define ADC_TR_HT_BITOFFSET_POS            (16U) /* Value equivalent to POSITION_VAL(ADC_TR_HT) */

/** @defgroup ADC_LL_EC_RESOLUTION  ADC instance - Resolution
 * @{
 */
#define LL_ADC_RESOLUTION_12B              0x00000000U                         /*!< ADC resolution 12 bits */
#define LL_ADC_RESOLUTION_10B              (                ADC_CR1_RES_0)     /*!< ADC resolution 10 bits */
#define LL_ADC_RESOLUTION_8B               (ADC_CR1_RES_1                )     /*!< ADC resolution  8 bits */
#define LL_ADC_RESOLUTION_6B               (ADC_CR1_RES_1 | ADC_CR1_RES_0)     /*!< ADC resolution  6 bits */


// -----------------------------------------------------------------------------------------------------------
/**
 * @brief  Helper macro to convert the ADC conversion data from
 *         a resolution to another resolution.
 * @param  __DATA__ ADC conversion data to be converted
 * @param  __ADC_RESOLUTION_CURRENT__ Resolution of to the data to be converted
 *         This parameter can be one of the following values:
 *         @arg @ref LL_ADC_RESOLUTION_12B
 *         @arg @ref LL_ADC_RESOLUTION_10B
 *         @arg @ref LL_ADC_RESOLUTION_8B
 *         @arg @ref LL_ADC_RESOLUTION_6B
 * @param  __ADC_RESOLUTION_TARGET__ Resolution of the data after conversion
 *         This parameter can be one of the following values:
 *         @arg @ref LL_ADC_RESOLUTION_12B
 *         @arg @ref LL_ADC_RESOLUTION_10B
 *         @arg @ref LL_ADC_RESOLUTION_8B
 *         @arg @ref LL_ADC_RESOLUTION_6B
 * @retval ADC conversion data to the requested resolution
 */
#define __LL_ADC_CONVERT_DATA_RESOLUTION(__DATA__, __ADC_RESOLUTION_CURRENT__, __ADC_RESOLUTION_TARGET__) \
  (((__DATA__)                                                                 \
    << ((__ADC_RESOLUTION_CURRENT__) >> (ADC_CR1_RES_BITOFFSET_POS - 1U)))     \
   >> ((__ADC_RESOLUTION_TARGET__) >> (ADC_CR1_RES_BITOFFSET_POS - 1U))        \
  )

/**
 * @brief  Helper macro to calculate analog reference voltage (Vref+)
 *         (unit: mVolt) from ADC conversion data of internal voltage
 *         reference VrefInt.
 * @note   Computation is using VrefInt calibration value
 *         stored in system memory for each device during production.
 * @note   This voltage depends on user board environment: voltage level
 *         connected to pin Vref+.
 *         On devices with small package, the pin Vref+ is not present
 *         and internally bonded to pin Vdda.
 * @note   On this STM32 serie, calibration data of internal voltage reference
 *         VrefInt corresponds to a resolution of 12 bits,
 *         this is the recommended ADC resolution to convert voltage of
 *         internal voltage reference VrefInt.
 *         Otherwise, this macro performs the processing to scale
 *         ADC conversion data to 12 bits.
 * @param  __VREFINT_ADC_DATA__: ADC conversion data (resolution 12 bits)
 *         of internal voltage reference VrefInt (unit: digital value).
 * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC_RESOLUTION_12B
 *         @arg @ref LL_ADC_RESOLUTION_10B
 *         @arg @ref LL_ADC_RESOLUTION_8B
 *         @arg @ref LL_ADC_RESOLUTION_6B
 * @retval Analog reference voltage (unit: mV)
 */
#define __LL_ADC_CALC_VREFANALOG_VOLTAGE(__VREFINT_ADC_DATA__,\
                                         __ADC_RESOLUTION__)                   \
  (((uint32_t)(*VREFINT_CAL_ADDR) * VREFINT_CAL_VREF)                          \
    / __LL_ADC_CONVERT_DATA_RESOLUTION((__VREFINT_ADC_DATA__),                 \
                                       (__ADC_RESOLUTION__),                   \
                                       LL_ADC_RESOLUTION_12B)                  \
  )

/**
 * @brief  Helper macro to calculate the temperature (unit: degree Celsius)
 *         from ADC conversion data of internal temperature sensor.
 * @note   Computation is using temperature sensor calibration values
 *         stored in system memory for each device during production.
 * @note   Calculation formula:
 *           Temperature = ((TS_ADC_DATA - TS_CAL1)
 *                           * (TS_CAL2_TEMP - TS_CAL1_TEMP))
 *                         / (TS_CAL2 - TS_CAL1) + TS_CAL1_TEMP
 *           with TS_ADC_DATA = temperature sensor raw data measured by ADC
 *                Avg_Slope = (TS_CAL2 - TS_CAL1)
 *                            / (TS_CAL2_TEMP - TS_CAL1_TEMP)
 *                TS_CAL1   = equivalent TS_ADC_DATA at temperature
 *                            TEMP_DEGC_CAL1 (calibrated in factory)
 *                TS_CAL2   = equivalent TS_ADC_DATA at temperature
 *                            TEMP_DEGC_CAL2 (calibrated in factory)
 *         Caution: Calculation relevancy under reserve that calibration
 *                  parameters are correct (address and data).
 *                  To calculate temperature using temperature sensor
 *                  datasheet typical values (generic values less, therefore
 *                  less accurate than calibrated values),
 *                  use helper macro @ref __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS().
 * @note   As calculation input, the analog reference voltage (Vref+) must be
 *         defined as it impacts the ADC LSB equivalent voltage.
 * @note   Analog reference voltage (Vref+) must be either known from
 *         user board environment or can be calculated using ADC measurement
 *         and ADC helper macro @ref __LL_ADC_CALC_VREFANALOG_VOLTAGE().
 * @note   On this STM32 serie, calibration data of temperature sensor
 *         corresponds to a resolution of 12 bits,
 *         this is the recommended ADC resolution to convert voltage of
 *         temperature sensor.
 *         Otherwise, this macro performs the processing to scale
 *         ADC conversion data to 12 bits.
 * @param  __VREFANALOG_VOLTAGE__  Analog reference voltage (unit: mV)
 * @param  __TEMPSENSOR_ADC_DATA__ ADC conversion data of internal
 *                                 temperature sensor (unit: digital value).
 * @param  __ADC_RESOLUTION__      ADC resolution at which internal temperature
 *                                 sensor voltage has been measured.
 *         This parameter can be one of the following values:
 *         @arg @ref LL_ADC_RESOLUTION_12B
 *         @arg @ref LL_ADC_RESOLUTION_10B
 *         @arg @ref LL_ADC_RESOLUTION_8B
 *         @arg @ref LL_ADC_RESOLUTION_6B
 * @retval Temperature (unit: degree Celsius)
 */
#define __LL_ADC_CALC_TEMPERATURE(__VREFANALOG_VOLTAGE__,\
                                  __TEMPSENSOR_ADC_DATA__,\
                                  __ADC_RESOLUTION__)                              \
  (((( ((int32_t)((__LL_ADC_CONVERT_DATA_RESOLUTION((__TEMPSENSOR_ADC_DATA__),     \
                                                    (__ADC_RESOLUTION__),          \
                                                    LL_ADC_RESOLUTION_12B)         \
                   * (__VREFANALOG_VOLTAGE__))                                     \
                  / TEMPSENSOR_CAL_VREFANALOG)                                     \
        - (int32_t) *TEMPSENSOR_CAL1_ADDR)                                         \
     ) * (int32_t)(TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP)                    \
    ) / (int32_t)((int32_t)*TEMPSENSOR_CAL2_ADDR - (int32_t)*TEMPSENSOR_CAL1_ADDR) \
   ) + TEMPSENSOR_CAL1_TEMP                                                        \
  )

/**
 * @brief  Helper macro to define the ADC conversion data full-scale digital
 *         value corresponding to the selected ADC resolution.
 * @note   ADC conversion data full-scale corresponds to voltage range
 *         determined by analog voltage references Vref+ and Vref-
 *         (refer to reference manual).
 * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC_RESOLUTION_12B
 *         @arg @ref LL_ADC_RESOLUTION_10B
 *         @arg @ref LL_ADC_RESOLUTION_8B
 *         @arg @ref LL_ADC_RESOLUTION_6B
 * @retval ADC conversion data equivalent voltage value (unit: mVolt)
 */
#define __LL_ADC_DIGITAL_SCALE(__ADC_RESOLUTION__)                             \
  (0xFFFU >> ((__ADC_RESOLUTION__) >> (ADC_CR1_RES_BITOFFSET_POS - 1U)))

/**
 * @brief  Helper macro to calculate the voltage (unit: mVolt)
 *         corresponding to a ADC conversion data (unit: digital value).
 * @note   Analog reference voltage (Vref+) must be either known from
 *         user board environment or can be calculated using ADC measurement
 *         and ADC helper macro @ref __LL_ADC_CALC_VREFANALOG_VOLTAGE().
 * @param  __VREFANALOG_VOLTAGE__ Analog reference voltage (unit: mV)
 * @param  __ADC_DATA__ ADC conversion data (resolution 12 bits)
 *                       (unit: digital value).
 * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
 *         @arg @ref LL_ADC_RESOLUTION_12B
 *         @arg @ref LL_ADC_RESOLUTION_10B
 *         @arg @ref LL_ADC_RESOLUTION_8B
 *         @arg @ref LL_ADC_RESOLUTION_6B
 * @retval ADC conversion data equivalent voltage value (unit: mVolt)
 */
#define __LL_ADC_CALC_DATA_TO_VOLTAGE(__VREFANALOG_VOLTAGE__,\
                                      __ADC_DATA__,\
                                      __ADC_RESOLUTION__)                      \
  ((__ADC_DATA__) * (__VREFANALOG_VOLTAGE__)                                   \
   / __LL_ADC_DIGITAL_SCALE(__ADC_RESOLUTION__)                                \
  )

