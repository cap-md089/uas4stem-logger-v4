#include "./commands.h"
#include <cstring>

void command_function_arm(Connection* conn, CurrentState* cs) {
	if (!cs->get_armed()) {
		conn->toggle_arm();
	}
}
