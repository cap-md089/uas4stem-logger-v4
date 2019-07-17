#pragma once
#ifndef _CURRENTSTATE_H_
#define _CURRENTSTATE_H_  

#include <string>
#include <vector>
#include <time.h>

#define IDEAL_PACKET_SIZE 101

#define UAV_MODE_UNDEFINED	(0)
#define UAV_MODE_AUTO		(1)
#define UAV_MODE_LOITER		(2)
#define UAV_MODE_ALTHOLD	(3)
#define UAV_MODE_GUIDED		(4)
#define UAV_MODE_RTL		(5)

typedef struct RecordedCoordinates {
	double latitude;
	double longitude;
} RecordedCoordinates;

class CurrentState {
	public:
		CurrentState();
		double get_latitude() const;
		double get_longitude() const;
		double get_altitude() const;
		double get_roll() const;
		double get_pitch() const;
		double get_yaw() const;
		unsigned int get_time_in_air() const;
		float get_battery_voltage() const;
		float get_battery_remaining() const;
		float get_ground_speed() const;
		float get_throttle() const;
		float get_dist_to_home() const;
		float get_vertical_speed() const;
		float get_rtl_speed() const;
		float get_rtl_land_speed() const;
		float get_time_required_for_landing() const;
		uint8_t get_waypoint_number() const;

		bool get_armed() const;

		bool get_flying() const;
		int get_battery_timer() const;
		bool get_continuing_flight() const;

		void continue_flight();

		bool get_recording();
		void start_recording(float left_percent, float right_percent);
		RecordedCoordinates* stop_recording();

		/**
		 * Updates state to match input
		 *
		 * The return value contains the status code:
		 *	0 for success
		 *  1 for incorrect length
		 *  2 for invalid header
		 */
		int update(const char*, int);

		void (*flying_callback)(CurrentState*) = NULL;
		void (*landed_callback)(CurrentState*, bool) = NULL;
		void (*update_callback)(CurrentState*) = NULL;
		void (*start_recording_callback)(CurrentState*) = NULL;
		void (*stop_recording_callback)(CurrentState*, RecordedCoordinates*) = NULL;

		void force_takeoff();
		void force_land();
	private:
		void takeoff();
		void land();

		unsigned int previous_time_in_air = 0;
		unsigned int ptime_in_air = 0;

		double latitude;
		double longitude;
		double altitude;
		double roll;
		double pitch;
		double yaw;
		unsigned int time_in_air;
		float battery_voltage;
		float battery_remaining;
		float ground_speed;
		float throttle;
		float dist_to_home;
		float vertical_speed;
		float rtl_speed;
		float rtl_land_speed;
		float time_required_for_landing;
		bool armed;
		uint8_t waypoint_number;
		uint8_t uav_mode;

		time_t takeoff_timestamp;
		bool flying;
		bool continuing_flight;

		bool recording_coordinates;
		float left_percent;
		float forward_percent;
		std::vector<RecordedCoordinates*> coordinates_being_recorded;
};

#endif
