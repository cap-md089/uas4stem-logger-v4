#include "CurrentState.h"
#include "../math/Vector3D.h"
#include <stdlib.h>
#include <string>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <inttypes.h>
#include <time.h>

const char header[5] = {
	'C', 'S', 'D', 'A', 'T'
};

const char footer[5] = {
	'C', 'S', 'E', 'N', 'D'
};

CurrentState::CurrentState() {
	flying_callback = NULL;
	landed_callback = NULL;
	update_callback = NULL;
	previous_time_in_air = 0;
	takeoff_timestamp = 0;
}

int CurrentState::update(const char* input, int length) {
	if (length != IDEAL_PACKET_SIZE) {
		return 1;
	}

	if (
		std::memcmp(input,			header, 5) != 0 ||
		std::memcmp(input+ 0x5E,	footer, 5) != 0
	) {
		return 2;
	}

	const char* input_data = input + 5;
	int ptime_in_air = get_time_in_air();

	//std::memcpy(&time_in_air,					input_data,			4);
	std::memcpy(&latitude,						input_data + 0x04,	8);
	std::memcpy(&longitude,						input_data + 0x0C,	8);
	std::memcpy(&battery_voltage,				input_data + 0x14,	4);
	std::memcpy(&battery_remaining,				input_data + 0x18,	4);
	std::memcpy(&altitude,						input_data + 0x1C,	8);
	std::memcpy(&ground_speed,					input_data + 0x24,	4);
	std::memcpy(&throttle,						input_data + 0x28,	4);
	std::memcpy(&dist_to_home,					input_data + 0x2C,	4);
	std::memcpy(&vertical_speed,				input_data + 0x30,	4);
	std::memcpy(&rtl_speed,						input_data + 0x34,	4);
	std::memcpy(&rtl_land_speed,				input_data + 0x38,	4);
	std::memcpy(&roll,							input_data + 0x3C,	8);
	std::memcpy(&yaw,							input_data + 0x44,	8);
	std::memcpy(&pitch,							input_data + 0x4C,	8);
	std::memcpy(&time_required_for_landing,		input_data + 0x54,	4);
	std::memcpy(&armed,							input_data + 0x58,	1);

	if (
		(throttle > 12 || ground_speed > 3) &&
		armed &&
		!flying
	) {
		flying = true;
		takeoff_timestamp = time(NULL);
		if (flying_callback != NULL) (*flying_callback)(this);
	} else if (
		((throttle < 12 && ground_speed < 3) || !armed) &&
		flying
	) {
		flying = false;
		if (landed_callback != NULL) (*landed_callback)(this, continuing_flight);

		if (continuing_flight) {
			previous_time_in_air += ptime_in_air;
			continuing_flight = false;
		} else {
			previous_time_in_air = 0;
		}
	}

	// Time in air is copied last, as it is used for the continuing flight part

	if (update_callback != NULL) (*update_callback)(this);

	if (recording_coordinates) {
		double x, y;

		get_xy_view_of_uav(
			&x, &y,
			roll, pitch, yaw,
			altitude,
			latitude, longitude,
			(double)left_percent, (double)right_percent
		);

		RecordedCoordinates* rec = (RecordedCoordinates*)malloc(sizeof(RecordedCoordinates));
		rec->latitude = x;
		rec->longitude = y;

		latitude = x;
		longitude = y;

		coordinates_being_recorded.push_back(rec);
	}

	return 0;
}

unsigned int CurrentState::get_time_in_air() const {
	if (!flying) {
		return 0;
	}
	return time(NULL) - takeoff_timestamp;
}

double CurrentState::get_latitude() const {
	return latitude;
}

double CurrentState::get_longitude() const {
	return longitude;
}

float CurrentState::get_battery_voltage() const {
	return battery_voltage;
}

float CurrentState::get_battery_remaining() const {
	return battery_remaining;
}

double CurrentState::get_altitude() const {
	return altitude;
}

float CurrentState::get_ground_speed() const {
	return ground_speed;
}

float CurrentState::get_throttle() const {
	return throttle;
}

float CurrentState::get_dist_to_home() const {
	return dist_to_home;
}

float CurrentState::get_vertical_speed() const {
	return vertical_speed;
}

float CurrentState::get_rtl_speed() const {
	return rtl_speed;
}

float CurrentState::get_rtl_land_speed() const {
	return rtl_land_speed;
}

double CurrentState::get_roll() const {
	return roll;
}

double CurrentState::get_pitch() const {
	return pitch;
}

double CurrentState::get_yaw() const {
	return yaw;
}

bool CurrentState::get_armed() const {
	return armed;
}

bool CurrentState::get_flying() const {
	return flying;
}

int CurrentState::get_battery_timer() const {
	return previous_time_in_air + get_time_in_air();
}

bool CurrentState::get_continuing_flight() const {
	return continuing_flight;
}

float CurrentState::get_time_required_for_landing() const {
	return time_required_for_landing;
}

void CurrentState::continue_flight() {
	continuing_flight = true;
}

bool CurrentState::get_recording() {
	return recording_coordinates;
}

void CurrentState::start_recording(float lp, float rp) {
	recording_coordinates = true;
	left_percent = lp;
	right_percent = rp;
}

RecordedCoordinates* CurrentState::stop_recording() {
	RecordedCoordinates* returnValue = (RecordedCoordinates*)malloc(sizeof(RecordedCoordinates));

	double x = 0, y = 0;

	if (coordinates_being_recorded.size() != 0) {
		for (unsigned int i = 0; i < coordinates_being_recorded.size(); i++) {
			RecordedCoordinates* record = coordinates_being_recorded.at(i);

			x += record->latitude;
			y += record->longitude;

			free(record);
		}

		x /= coordinates_being_recorded.size();
		y /= coordinates_being_recorded.size();
	}

	coordinates_being_recorded.erase(
		coordinates_being_recorded.begin(),
		coordinates_being_recorded.end()
	);

	returnValue->latitude = x;
	returnValue->longitude = y;

	return returnValue;
}