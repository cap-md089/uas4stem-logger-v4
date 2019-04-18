#include "./Session.h"
#include "../config/config.h"

#include <cstring>
#include <string>
#include <fstream>
#include <time.h>

#include <iostream>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#define MAX_TIME_LENGTH 40

void clear_flight(CurrentFlight* flight) {
	flight->battery_voltages.clear();
	flight->targets.clear();
}

void save_flight(Configuration* conf, CurrentFlight* flight, int time_in_air, int battery_id) {
	char location[MAX_PATH];
	conf->get_data_dir_location(location);
	char file_name[MAX_TIME_LENGTH];
	std::string file_name2;

	time_t rawtime;
	struct tm* time_info;
	time(&rawtime);
	time_info = localtime(&rawtime);

	strftime(file_name, MAX_TIME_LENGTH, "Flight %a %Y-%m-%d %H-%M", time_info);

	strcat(location, "\\");
	strcat(location, file_name);
	strcat(location, ".txt");
	file_name2 = location;

	std::cout << "Saving to: " << location << std::endl;
	std::cout << "Printing report: " << std::endl;

	std::ofstream fs;
	fs.open(location);

	fs << file_name << std::endl;
	fs << "Time in air: " << time_in_air << ";" << std::endl;
	fs << "Battery ID (0-based): " << battery_id << ";" << std::endl;
	fs << std::endl;
	std::cout << file_name << std::endl;
	std::cout << "Time in air: " << time_in_air << ";" << std::endl;
	std::cout << "Battery ID (0-based): " << battery_id << ";" << std::endl;
	std::cout << std::endl;
	
	fs << "--- Targets found ---" << std::endl;
	std::cout << "--- Targets found ---" << std::endl;

	for (auto &a : flight->targets) {
		fs << "(" << a->latitude << ", " << a->longitude << "): \"" << a->description << "\"; found at " << a->time_in_air << ";" << std::endl;
		std::cout << "(" << a->latitude << ", " << a->longitude << "): \"" << a->description << "\"; found at " << a->time_in_air << ";" << std::endl;
	}

	fs << std::endl;
	fs << "--- Battery voltages ---" << std::endl;

	std::cout << std::endl;
	std::cout << "--- Battery voltages ---" << std::endl;

	double v;
	unsigned int i = 0;
	for (; i < flight->battery_voltages.size() - 1; i++) {
		v = flight->battery_voltages.at(i);
		fs << (i * 10) << "sec: " << v << std::endl;
		std::cout << (i * 10) << "sec: " << v << std::endl;
	}

	v = flight->battery_voltages.at(i);
	fs << time_in_air << "sec: " << v << std::endl;
	std::cout << time_in_air << "sec: " << v << std::endl;

	fs << std::endl;
	fs << "--- End report ---";
	std::cout << std::endl;
	std::cout << "--- End report ---" << std::endl;

	fs.close();
}