#include "./CurrentState.h"
#include "../math/Vector3D.h"
#include <stdlib.h>
#include <string>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <inttypes.h>
#include <time.h>

// Used for playing a beep when the waypoint number changes
// because I forget to keep track...
#ifdef _WIN32
#include <Windows.h>
#include <mmsystem.h>
#endif

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
	start_recording_callback = NULL;
	stop_recording_callback = NULL;

	previous_time_in_air = 0;
	flying = false;
	takeoff_timestamp = 0;
	continuing_flight = false;
	recording_coordinates = false;
	std::vector<RecordedCoordinates*> coordinates_being_recorded;
	recording_coordinates = false;

	latitude = 0;
	longitude = 0;
	altitude = 0;
	roll = 0;
	pitch = 0;
	yaw = 0;
	time_in_air = 0;
	battery_voltage = 0;
	battery_remaining = 0;
	ground_speed = 0;
	throttle = 0;
	dist_to_home = 0;
	vertical_speed = 0;
	rtl_speed = 0;
	rtl_land_speed = 0;
	time_required_for_landing = 0;
	armed = false;
	waypoint_number = 0;
	uav_mode = UAV_MODE_UNDEFINED;
}

int CurrentState::update(const char* input, int length) {
	if (length != IDEAL_PACKET_SIZE) {
		return 1;
	}

	if (
		std::memcmp(input,			header, 5) != 0 ||
		std::memcmp(input+ 0x60,	footer, 5) != 0
	) {
		return 2;
	}

	const char* input_data = input + 5;
	ptime_in_air = get_time_in_air();
	int wpno = get_waypoint_number();

	std::memcpy(&latitude,						input_data,			8);
	std::memcpy(&longitude,						input_data + 0x08,	8);
	std::memcpy(&altitude,						input_data + 0x10,	8);
	std::memcpy(&roll,							input_data + 0x18,	8);
	std::memcpy(&yaw,							input_data + 0x20,	8);
	std::memcpy(&pitch,							input_data + 0x28,	8);
	std::memcpy(&time_in_air,					input_data + 0x30,	4);
	std::memcpy(&battery_voltage,				input_data + 0x34,	4);
	std::memcpy(&battery_remaining,				input_data + 0x38,	4);
	std::memcpy(&ground_speed,					input_data + 0x3C,	4);
	std::memcpy(&throttle,						input_data + 0x40,	4);
	std::memcpy(&dist_to_home,					input_data + 0x44,	4);
	std::memcpy(&vertical_speed,				input_data + 0x48,	4);
	std::memcpy(&rtl_speed,						input_data + 0x4C,	4);
	std::memcpy(&rtl_land_speed,				input_data + 0x50,	4);
	std::memcpy(&time_required_for_landing,		input_data + 0x54,	4);
	std::memcpy(&armed,							input_data + 0x58,	1);
	std::memcpy(&waypoint_number,				input_data + 0x59,	1);
	std::memcpy(&uav_mode,						input_data + 0x5A,	1);

	std::cout << uav_mode << std::endl;

	if (
		(throttle > 12 || ground_speed > 3 || uav_mode == UAV_MODE_AUTO || altitude > 9.0) &&
		armed &&
		!flying
	) {
		takeoff();
	} else if (
		((throttle < 12 && ground_speed < 3 && uav_mode != UAV_MODE_AUTO && altitude < 9.0) || !armed) &&
		flying
	) {
		land();
	}

	if (update_callback != NULL) (*update_callback)(this);

	if (recording_coordinates) {
		double x, y;

		get_xy_view_of_uav(
			&x, &y,
			roll, pitch, yaw,
			altitude,
			latitude, longitude,
			(double)left_percent, (double)forward_percent
		);

		RecordedCoordinates* rec = new RecordedCoordinates;
		rec->latitude = x;
		rec->longitude = y;

		latitude = x;
		longitude = y;

		coordinates_being_recorded.push_back(rec);
	}

	if (wpno != get_waypoint_number()) {
#ifdef _WIN32
		PlaySound(TEXT("SystemStart"), NULL, SND_ALIAS);
#endif
	}

	return 0;
}

void CurrentState::land() {
	if (landed_callback != NULL) (*landed_callback)(this, continuing_flight);
	flying = false;

	if (continuing_flight) {
		previous_time_in_air += ptime_in_air;
		continuing_flight = false;
	} else {
		previous_time_in_air = 0;
	}
}

void CurrentState::force_land() {
	land();
}

void CurrentState::takeoff() {
	flying = true;
	takeoff_timestamp = time(NULL);
	if (flying_callback != NULL) (*flying_callback)(this);
}

void CurrentState::force_takeoff() {
	takeoff();
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

uint8_t CurrentState::get_waypoint_number() const {
	return waypoint_number;
}

void CurrentState::continue_flight() {
	continuing_flight = true;
}

bool CurrentState::get_recording() {
	return recording_coordinates;
}

void CurrentState::start_recording(float lp, float fp) {
	recording_coordinates = true;
	left_percent = lp;
	forward_percent = fp;
	if (start_recording_callback != NULL) start_recording_callback(this);
}

RecordedCoordinates* CurrentState::stop_recording() {
	RecordedCoordinates* returnValue = (RecordedCoordinates*)malloc(sizeof(RecordedCoordinates));

	if (!recording_coordinates) {
		return NULL;
	}

	recording_coordinates = false;

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

	if (stop_recording_callback != NULL) stop_recording_callback(this, returnValue);

	return returnValue;
}
