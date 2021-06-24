#ifndef COMMON_LOGGER_LOGTARGETLIST_H_
#define COMMON_LOGGER_LOGTARGETLIST_H_

#include "logger/include/Logger.h"
#include "utils/include/List.h"

class LogTargetList : public LogTarget {
public:
	void add(LogTarget *target) {
		targets.add(target);
	}

	virtual void send(const uint8_t *data, const uint16_t len) {
		for(int index = 0; index < targets.getSize(); index++) {
			LogTarget *target = targets.get(index);
			if(target == NULL) {
				return;
			}
			target->send(data, len);
		}
	}

	void clear() {
		targets.clearAndFree();
	}

private:
	List<LogTarget> targets;
};

#endif
