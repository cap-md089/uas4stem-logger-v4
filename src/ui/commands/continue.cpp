#include "./commands.h"
#include <cstring>

void command_function_continue_flight(CurrentState* cs) {
	cs->continue_flight();
}
