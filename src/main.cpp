#include "data/CurrentState.h"
#include "connection/Connection.h"
#include "config/config.h"
#include "ui/ui.h"
#include <vector>
#include <string>
#include <cstring>
#include <iostream>

int main (int argc, char **argv) {
	std::vector<std::string> user_log;
	CurrentState cs;
	Connection conn;
	Configuration conf;

	int setup_status = conn.setup(&cs, &user_log);
	if (setup_status != EXIT_SUCCESS) {
		return setup_status;
	}

	conf.open();

	int gui_status = start_gui(argc, argv, &cs, &user_log, &conn, &conf);

	conn.stop();

	return gui_status;
}
