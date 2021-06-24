#ifndef __STRING_FINDER_H
#define __STRING_FINDER_H

#include "utils/include/StringBuilder.h"

class StringFinder {
	private:
	enum State {
		findStart,
		findStop,
		complete
	};
	
	StringBuilder *start;
	StringBuilder *stop;
	StringBuilder *between;
	State state;
	uint8_t index;
	
	public:
	StringFinder(const char *start, const char *stop) {
		this->start = new StringBuilder(start);
		this->stop = new StringBuilder(stop);
		this->between = NULL;
		this->state = findStart;
		this->index = 0;
	}
	
	~StringFinder() {
		if (start)		delete start;
		if (stop)			delete stop;
		if (between)	delete between;
	}
	
	StringBuilder *getBetween() {
		return between;
	}
	
	inline bool isCompleted() {
		return state == complete;
	}
	
	void read(uint8_t b) {
		if (state == findStart) {
			if (b == (*start)[index]) {
				if (++index == start->getLen()) {
					state = findStop;
					index = 0;
					between = new StringBuilder();
				}
			} else {
					index = 0;
			}
		} else if (state == findStop) {
			between->add(b);
			if (b == (*stop)[index]) {
				if (++index == stop->getLen()) {
					between->setLen(between->getLen()-stop->getLen());
					state = complete;
				}
			} else {
			index = 0;
		}
	} else {
			// Ignore
			
		}
	}	
	
	bool read(const char *str) {
		while(*str) {
			char c = str[0];
			read(c);
			str++;
			if (isCompleted()) return true;
		}
		return false;
	}

};


#endif

