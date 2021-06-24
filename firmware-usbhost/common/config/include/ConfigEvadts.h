#ifndef COMMON_CONFIG_EVADTS_H_
#define COMMON_CONFIG_EVADTS_H_

#ifndef NEW_CONFIG
#include "common/config/v3/Config3ConfigIniter.h"
#include "common/config/v3/Config3ConfigParser.h"
#include "common/config/v3/Config3ConfigGenerator.h"
#include "common/config/v3/Config3AuditFiller.h"
#include "common/config/v3/Config3AuditGenerator.h"

typedef Config3ConfigIniter ConfigConfigIniter;
typedef Config3ConfigParser ConfigConfigParser;
typedef Config3ConfigGenerator ConfigConfigGenerator;
typedef Config3AuditGenerator ConfigAuditGenerator;
typedef Config3AuditGenerator ConfigAuditGeneratorVendmax;
#else
#include "common/config/v4/Config4ConfigIniter.h"
#include "common/config/v4/Config4ConfigParser.h"
#include "common/config/v4/Config4ConfigGenerator.h"
//#include "common/config/v4/Config4AuditFiller.h"
#include "common/config/v4/Config4AuditGenerator.h"
#include "common/config/v4/Config4MeiAuditGenerator.h"

typedef Config4ConfigIniter ConfigConfigIniter;
typedef Config4ConfigParser ConfigConfigParser;
typedef Config4ConfigGenerator ConfigConfigGenerator;
typedef Config4AuditGenerator ConfigAuditGenerator;
typedef Config4MeiAuditGenerator ConfigAuditGeneratorVendmax;
#endif

#endif
