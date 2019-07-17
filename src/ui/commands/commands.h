#pragma once

#include "../../connection/Connection.h"
#include "../../data/CurrentState.h"

void (*command_function)(int, char**, Connection*, CurrentState*);
