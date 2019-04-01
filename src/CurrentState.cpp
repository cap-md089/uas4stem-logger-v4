#include <stdlib.h>
#include "CurrentState.h"
#include <iostream>

const char header[5] = {
	'C', 'S', 'D', 'A', 'T'
};

const char footer[5] = {
	'C', 'S', 'E', 'N', 'D'
};

int parseCurrentState(CurrentState* state, std::string input) {
	/*if (input.length() != 91) {
		return 1;
	}*/

	char* input_data = input.data();

	if (
		std::memcmp(input_data,			header, 5) != 0 ||
		std::memcmp(input_data + 80,	footer, 5) != 0
	) {
		return 2;
	}

	input_data += 5;

	std::memcpy(&(state->timeInAir),		input_data,			4);
	std::memcpy(&(state->latitude),			input_data + 0x04,	8);
	std::memcpy(&(state->longitude),		input_data + 0x0C,	8);
	std::memcpy(&(state->batteryVoltage),	input_data + 0x14,	4);
	std::memcpy(&(state->batteryRemaining),	input_data + 0x18,	4);
	std::memcpy(&(state->altitude),			input_data + 0x1C,	4);
	std::memcpy(&(state->groundspeed),		input_data + 0x20,	4);
	std::memcpy(&(state->throttle),			input_data + 0x24,	4);
	std::memcpy(&(state->distToHome),		input_data + 0x28,	4);
	std::memcpy(&(state->verticalSpeed),	input_data + 0x2C,	4);
	std::memcpy(&(state->rtlSpeed),			input_data + 0x30,	4);
	std::memcpy(&(state->rtlLandSpeed),		input_data + 0x34,	4);
	std::memcpy(&(state->roll),				input_data + 0x38,	8);
	std::memcpy(&(state->pitch),			input_data + 0x40,	8);
	std::memcpy(&(state->yaw),				input_data + 0x48,	8);
	std::memcpy(&(state->armed),			input_data + 0x49,	1);

	return 0;
}