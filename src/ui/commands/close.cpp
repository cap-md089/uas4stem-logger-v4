#include "../../connection/Connection.h"

void command_function_close(int argc, char** argv, Connection* conn) {
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == 'g') {
			conn->close_golf_bottle();
		}

		if (argv[i][0] == 'c') {
			conn->close_charlie_bottle();
		}
	}
}
