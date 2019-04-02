#ifndef _GUI_H_
#define _GUI_H_

#include "../data/CurrentState.h"
#include <gtk/gtk.h>
#include <vector>
#include <string>
#include <cstring>

static void activate (GtkApplication* app, gpointer user_data);

int start_gui (int argc, char **argv, CurrentState* cs, const std::vector<std::string>* log);

#endif
