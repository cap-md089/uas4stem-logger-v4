#include "./config.h"
#include <fstream>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <iostream>

#ifdef _WIN32
#include <ShlObj.h>
#include <Windows.h>
#include <Shlwapi.h>
#endif

int Configuration::open() {
#ifdef _WIN32
	char location[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, location);
	PathAppend(location, "uas4stem-logger");
	if (!CreateDirectory(location, NULL) && !(ERROR_ALREADY_EXISTS == GetLastError())) {
		return 1;
	}
	PathAppend(location, "config");

	std::ifstream fs;
	fs.open(location, std::ifstream::in);

	unsigned int length = fs.tellg();

	// File doesn't exist or is wrong
	if (length != 4) {
		max_flight_time = 480;

		return 0;
	}
	
	char header[4];
	unsigned int max_flight_time;

	fs.read(header, 4);

	std::memcpy(&max_flight_time,	header,			4);

	fs.close();
#else
	max_flight_time = 480;
#endif
	return 0;
}

int Configuration::save() {
#ifdef _WIN32
	char location[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, location);
	PathAppend(location, "uas4stem-logger");
	if (!CreateDirectory(location, NULL) && !(ERROR_ALREADY_EXISTS == GetLastError())) {
		return 1;
	}
	PathAppend(location, "config");

	std::ofstream fs;
	fs.open(location, std::ofstream::out | std::ofstream::trunc);

	char data[4];

	std::memcpy(data, &max_flight_time, 4);

	fs.write(data, 4);
	fs.close();
#endif
	return 0;
}

unsigned int Configuration::get_max_flight_time() {
	return max_flight_time;
}

void Configuration::set_max_flight_time(unsigned int flight_time) {
	max_flight_time = flight_time;
}

void Configuration::get_data_dir_location(char* location) {
#ifdef _WIN32
	SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, location);
	PathAppend(location, "uas4stem-logger");
	CreateDirectory(location, NULL);
#else
	const char* path = "/home/arioux/Documents/uas4stem-logger\0";
	std::memcpy(location, path, 39);
#endif
}