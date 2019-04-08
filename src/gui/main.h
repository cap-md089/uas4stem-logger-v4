#ifndef _GUI_H_
#define _GUI_H_

#include "../data/CurrentState.h"
#include "../connection/Connection.h"
#include <gtk/gtk.h>
#include <vector>
#include <string>
#include <cstring>

int start_gui (
	int argc,
	char **argv,
	CurrentState* cs,
	std::vector<std::string>* log,
	Connection* conn
);

#endif
