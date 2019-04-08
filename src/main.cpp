#include "gui/main.h"
#include "data/CurrentState.h"
#include "connection/Connection.h"
#include <vector>
#include <string>
#include <cstring>
#include <iostream>

std::vector<std::string> user_log;
CurrentState cs;
Connection conn;

int main (int argc, char **argv) {
	conn.setup(&cs, &user_log);

	int gui_status = start_gui(argc, argv, &cs, &user_log, &conn);

	conn.stop();

	return gui_status;
}
