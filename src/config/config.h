#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string>

class Configuration {
	public:
		/*
			Int returned is status code

			0: success
			1: error
		*/
		int open();
		int save();

		unsigned int get_max_flight_time();
		void set_max_flight_time(unsigned int);

		void get_data_dir_location(char*);
	private:
		unsigned int max_flight_time;
};

#endif