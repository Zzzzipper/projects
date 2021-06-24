#include "dex/include/AuditProduct.h"

#include "logger/include/Logger.h"

#include <string.h>

AuditProduct::AuditProduct() {

}

AuditProduct::~AuditProduct() {

}

bool AuditProduct::compareId(const char *id) {
	return this->id == id;
}
