#pragma once

#include "../../connection/Connection.h"
#include "../../data/CurrentState.h"

void command_function_arm(Connection*, CurrentState*);
void command_function_open(int argc, char** argv, Connection*);
void command_function_close(int argc, char** argv, Connection*);
void command_function_continue_flight(CurrentState*);
void command_function_disarm(Connection*, CurrentState*);
void command_function_set(int argc, char** argv, char** pcurrent_target_label);
