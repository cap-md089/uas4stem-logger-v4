#ifndef _SESSION_H_
#define _SESSION_H_

#include <string>
#include <vector>
#include "../config/config.h"

typedef struct RecordedTarget {
	double latitude;
	double longitude;
	std::string description;
	int time_in_air;
} RecordedTarget;

typedef struct CurrentFlight {
	std::vector<RecordedTarget*> targets;
	std::vector<double> battery_voltages;
} CurrentFlight;

void clear_flight(CurrentFlight*);

void save_flight(Configuration*, CurrentFlight*, int time_in_air, int battery_id);

#endif