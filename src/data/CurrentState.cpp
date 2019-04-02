#include "CurrentState.h"
#include <stdlib.h>
#include <string>
#include <cstring>

const char header[5] = {
	'C', 'S', 'D', 'A', 'T'
};

const char footer[5] = {
	'C', 'S', 'E', 'N', 'D'
};

int CurrentState::update(std::string input) {
	/*if (input.length() != 91) {
		return 1;
	}*/

	const char* input_data = input.data();

	if (
		std::memcmp(input_data,			header, 5) != 0 ||
		std::memcmp(input_data + 80,	footer, 5) != 0
	) {
		return 2;
	}

	input_data += 5;

	std::memcpy(&time_in_air,			input_data,			4);
	std::memcpy(&latitude,				input_data + 0x04,	8);
	std::memcpy(&longitude,				input_data + 0x0C,	8);
	std::memcpy(&battery_voltage,		input_data + 0x14,	4);
	std::memcpy(&battery_remaining,		input_data + 0x18,	4);
	std::memcpy(&altitude,				input_data + 0x1C,	4);
	std::memcpy(&ground_speed,			input_data + 0x20,	4);
	std::memcpy(&throttle,				input_data + 0x24,	4);
	std::memcpy(&dist_to_home,			input_data + 0x28,	4);
	std::memcpy(&vertical_speed,		input_data + 0x2C,	4);
	std::memcpy(&rtl_speed,				input_data + 0x30,	4);
	std::memcpy(&rtl_land_speed,		input_data + 0x34,	4);
	std::memcpy(&roll,					input_data + 0x38,	8);
	std::memcpy(&pitch,					input_data + 0x40,	8);
	std::memcpy(&yaw,					input_data + 0x48,	8);
	std::memcpy(&armed,					input_data + 0x49,	1);

	return 0;
}
