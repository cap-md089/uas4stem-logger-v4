#pragma once
#ifndef _CURRENTSTATE_H_
#define _CURRENTSTATE_H_  

#include <string>

class CurrentState {
	public:
		int get_time_in_air() const;
		double get_latitude() const;
		double get_longitude() const;
		float get_battery_voltage() const;
		float get_battery_remaining() const;
		float get_altitude() const;
		float get_ground_speed() const;
		float get_throttle() const;
		float get_dist_to_home() const;
		float get_vertical_speed() const;
		float get_rtl_speed() const;
		float get_rtl_land_speed() const;
		double get_roll() const;
		double get_pitch() const;
		double get_yaw() const;

		bool get_armed() const;

		bool get_flying() const;
		int get_battery_timer() const;
		bool get_continuing_flight() const;

		/**
		 * Updates state to match input
		 *
		 * The return value contains the status code:
		 *	0 for success
		 *  1 for incorrect length
		 *  2 for invalid header
		 */
		int update(std::string input);
	private:
		int time_in_air;
		double latitude;
		double longitude;
		float battery_voltage;
		float battery_remaining;
		float altitude;
		float ground_speed;
		float throttle;
		float dist_to_home;
		float vertical_speed;
		float rtl_speed;
		float rtl_land_speed;
		double roll;
		double pitch;
		double yaw;

		bool armed;

		bool flying;
		int battery_timer;
		bool continuing_flight;
};

#endif
