#ifndef _GUI_H_
#define _GUI_H_

#include "../data/CurrentState.h"
#include "../connection/Connection.h"
#include "../config/config.h"
#include <gtk/gtk.h>
#include <vector>
#include <string>
#include <cstring>

int start_gui (
	int,
	char**,
	CurrentState*,
	std::vector<std::string>*,
	Connection*,
	Configuration*
);

typedef struct RecordedTarget {
	double latitude;
	double longitude;
	std::string description;
	int time_in_air;
} RecordedTarget;

typedef struct CurrentFlight {
	std::vector<RecordedTarget> targets;
	std::vector<double> battery_voltages;
	int time_in_air;
	int battery_id;
} CurrentFlight;

#endif
