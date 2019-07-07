#ifndef _GUI_H_
#define _GUI_H_

#include "../data/CurrentState.h"
#include "../connection/Connection.h"
#include "../config/config.h"
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

#endif
