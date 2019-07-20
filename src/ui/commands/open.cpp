#include "../../connection/Connection.h"

void command_function_open(int argc, char** argv, Connection* conn) {
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == 'g') {
			conn->open_golf_bottle();
		}

		if (argv[i][0] == 'c') {
			conn->open_charlie_bottle();
		}
	}
}
