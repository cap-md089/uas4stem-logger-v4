#pragma once
#include "gui/main.h"
#include "data/CurrentState.h"
#include <vector>
#include <string>
#include <cstring>

std::vector<std::string> user_log;
CurrentState cs;

int main (int argc, char **argv) {
	return start_gui(argc, argv, &cs, &user_log);
}
