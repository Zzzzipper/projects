/*
 * TimeHelper.h
 *
 * Created: 14.05.2015 12:16:45
 *  Author: Administrator
 */ 


#ifndef TIMEHELPER_H_
#define TIMEHELPER_H_

#include "utils/StringBuilder.h"

class TimeHelper
{	
public: 
	
	// Переводим время в строку, в минимальную целую единицу времени.
	static String timeToSimpleString(unsigned long time_ms)
	{
		String res;
		if (time_ms % 1000 == 0)
		{
			time_ms /= 1000;
			if (time_ms % 60 == 0) {
				time_ms /= 60;
				if (time_ms % 60 == 0) {
					time_ms /= 60;
					res << time_ms << "ч";
				} else res << time_ms << "м";
				
			} else res << time_ms << "с";
		} else res << time_ms;
		
		return res;
	}	
	
	static String timeToString(unsigned long time_ms)
	{
		long sec = 1000l;
		long min = sec * 60l;
		long hour = min * 60l;
		long day = hour * 24l;
		
		String res;
		long d = time_ms / day;
		time_ms %= day;
		long h = time_ms / hour;
		time_ms %= hour;
		long m = time_ms / min;
		time_ms %= min;
		long s = time_ms / sec;
		
		if (d) res << d << "д ";
		if (h) res << h << "ч ";
		if (m) res << m << "м ";
		if (s) res << s << "с ";
		
		return res;
	}
	
	static bool timeStringToLong(String &str, long *result)
	{
		long res = 0;
		for(uint8_t i = 0; i < str.getLen(); i++) {
			char c = str[i];
			if (c == 'ч' || c == 'h') { res *= MIN(60); *result = res; return true; }
			if (c == 'м' || c == 'm') { res *= SEC(60); *result = res; return true; }
			if (c == 'с' || c == 's') { res *= SEC(1); *result = res; return true; }
			if (c >= '0' && c <= '9')  { res = (res * 10) + (c - '0'); }
			else return false;
		}
		
		 *result = res;
		return true;
	}
};




#endif /* TIMEHELPER_H_ */