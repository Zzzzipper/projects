#ifndef COMMON_CONFIG_MODEM_H_
#define COMMON_CONFIG_MODEM_H_

#include "config.h"
#ifndef NEW_CONFIG
#include "config/v3/Config3Modem.h"
#include "config/v3/event/Config3EventIterator.h"

#define CONFIG_EVENT_UNSET CONFIG3_EVENT_UNSET

typedef Config1Boot ConfigBoot;
typedef Config2Fiscal ConfigFiscal;
typedef Config2Cert ConfigCert;
typedef Config3EventList ConfigEventList;
typedef Config3Automat ConfigAutomat;
typedef Config3Price ConfigPrice;
typedef Config3ProductIterator ConfigProductIterator;
typedef Config3Modem ConfigModem;
typedef Config3EventIterator ConfigEventIterator;
#else
#include "config/v4/Config4Modem.h"
#include "config/v4/event/Config4EventIterator.h"

#define CONFIG_EVENT_UNSET CONFIG4_EVENT_UNSET

typedef Config1Boot ConfigBoot;
typedef Config2Fiscal ConfigFiscal;
typedef Config2Cert ConfigCert;
typedef Config4EventList ConfigEventList;
typedef Config4Automat ConfigAutomat;
typedef Config3Price ConfigPrice;
typedef Config4ProductIterator ConfigProductIterator;
typedef Config4Modem ConfigModem;
typedef Config4EventIterator ConfigEventIterator;
#endif

#endif
