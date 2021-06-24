#ifndef COMMON_CONFIG_REPAIRER_H_
#define COMMON_CONFIG_REPAIRER_H_

#ifndef NEW_CONFIG
#include "config/v3/Config3Repairer.h"

typedef Config3Repairer ConfigRepairer;
#else
#include "config/v4/Config4Repairer.h"

typedef Config4Repairer ConfigRepairer;
#endif

#endif
