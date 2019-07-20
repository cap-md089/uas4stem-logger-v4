#include "./commands.h"

void command_function_disarm(Connection* conn, CurrentState* cs) {
	if (cs->get_armed()) {
		conn->toggle_arm();
	}
}
