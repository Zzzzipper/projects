#pragma once

#include <stdint.h>

class Random
{
private:
	Random();
	~Random();

public:
	static Random &get();
	uint32_t getValue();
};
