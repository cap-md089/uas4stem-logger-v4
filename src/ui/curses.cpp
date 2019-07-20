#include "../data/CurrentState.h"
#include "../data/Session.h"
#include "../connection/Connection.h"
#include "../config/config.h"

#include "commands/commands.h"

#include <ncurses.h>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <time.h>
#include <math.h>
#include <fstream>
#include <stdint.h>

#define CH_LR_SIDE ('-')
#define CH_UD_SIDE ('|')
#define CH_CORNER ('+')

#define COLOR_SELECTED_MENU_ITEM (1)
#define COLOR_UNSELECTED_MENU_ITEM (2)
#define COLOR_NORMAL_TEXT (3)

#define MENU_MAIN (0)
#define MENU_CONFIGURATION (1)
#define MENU_FLIGHT_PLAN (2)
#define MENU_SCRABBLE (3)
#define MENU_QUIT (4)

static WINDOW* menu_window;
static WINDOW* main_window;
static WINDOW* command_window;

static Connection* dataConnection;
static CurrentState* currentState;
static Configuration* config;
static CurrentFlight flight;
static CurrentFlight overall_flight;

//static int flight_count = 0;
//static int timeout_id;
static char* current_target_label = NULL;
static char* battery_name = NULL;

static std::vector<bool> selected_coordinates;

static int current_selected_menu = 0;
static bool is_menu_selected = true;
static bool has_update = true;
static bool should_stop = false;

static bool is_entering_command = false;
static char* command_prompt = NULL;

static double camera_width(double alt) {
	return 2 * 1.04891304331  * alt;
}

static double camera_depth(double alt) {
	return 2 * 0.489130434783 * alt;
}

static void render_main_window(CurrentState*);

static void render_menu_selector(WINDOW* win) {
	wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');

	init_pair(COLOR_SELECTED_MENU_ITEM, COLOR_BLACK, COLOR_WHITE);
	init_pair(COLOR_UNSELECTED_MENU_ITEM, COLOR_WHITE, COLOR_BLACK);

	if (is_menu_selected) {
		wattrset(win, COLOR_PAIR(COLOR_SELECTED_MENU_ITEM));
	} else {
		wattron(win, A_UNDERLINE);
	}

	if (current_selected_menu == MENU_MAIN) {
		mvwaddstr(win, 1, 2, "Data");
		wattroff(win, A_UNDERLINE);
		wattrset(win, COLOR_PAIR(COLOR_UNSELECTED_MENU_ITEM));
		mvwaddstr(win, 1, 7, "Conf");
		mvwaddstr(win, 1, 12, "Coord");
		mvwaddstr(win, 1, 18, "Scrabble");
		mvwaddstr(win, 1, 27, "Quit");
	} else if (current_selected_menu == MENU_CONFIGURATION) {
		mvwaddstr(win, 1, 7, "Conf");
		wattroff(win, A_UNDERLINE);
		wattrset(win, COLOR_PAIR(COLOR_UNSELECTED_MENU_ITEM));
		mvwaddstr(win, 1, 2, "Data");
		mvwaddstr(win, 1, 12, "Coord");
		mvwaddstr(win, 1, 18, "Scrabble");
		mvwaddstr(win, 1, 27, "Quit");
	} else if (current_selected_menu == MENU_FLIGHT_PLAN) {
		mvwaddstr(win, 1, 12, "Coord");
		wattroff(win, A_UNDERLINE);
		wattrset(win, COLOR_PAIR(COLOR_UNSELECTED_MENU_ITEM));
		mvwaddstr(win, 1, 7, "Conf");
		mvwaddstr(win, 1, 2, "Data");
		mvwaddstr(win, 1, 18, "Scrabble");
		mvwaddstr(win, 1, 27, "Quit");
	} else if (current_selected_menu == MENU_SCRABBLE) {
		mvwaddstr(win, 1, 18, "Scrabble");
		wattroff(win, A_UNDERLINE);
		wattrset(win, COLOR_PAIR(COLOR_UNSELECTED_MENU_ITEM));
		mvwaddstr(win, 1, 7, "Conf");
		mvwaddstr(win, 1, 2, "Data");
		mvwaddstr(win, 1, 12, "Coord");
		mvwaddstr(win, 1, 27, "Quit");
	} else if (current_selected_menu == MENU_QUIT) {
		mvwaddstr(win, 1, 27, "Quit");
		wattroff(win, A_UNDERLINE);
		wattrset(win, COLOR_PAIR(COLOR_UNSELECTED_MENU_ITEM));
		mvwaddstr(win, 1, 7, "Conf");
		mvwaddstr(win, 1, 2, "Data");
		mvwaddstr(win, 1, 12, "Coord");
		mvwaddstr(win, 1, 18, "Scrabble");
	}

	wrefresh(win);
}

static void clear_command_bar(WINDOW* win) {
	mvwhline(win, 1, 1, ' ', COLS - 2);

	wrefresh(win);
	refresh();
}

static void render_command_bar(WINDOW* win) {
	clear_command_bar(win);

	mvwaddch(win, 1, 1, ':');
	mvwaddstr(win, 1, 2, command_prompt);

	wrefresh(win);
	refresh();
}

static void handle_command_bar_keypress(int ch, WINDOW* win) {
	size_t len = strlen(command_prompt);

	if (ch == 10) { // Newline, execute command
		int argc = 1;
		for (size_t i = 0; i < len; i++) {
			if (command_prompt[i] == ' ') {
				argc++;
			}
		}

		char** argv = new char*[argc];
		int cargc = 0;
		for (size_t i = 0; i < len; i++) {
			int arglen = 0;
			for (size_t j = i; j <= len; j++) {
				if (j == len || command_prompt[j] == ' ') {
					arglen = j - i;
					break;
				}
			}
			
			argv[cargc] = new char[arglen + 1];
			for (size_t j = i; j < i + arglen; j++) {
				argv[cargc][j - i] = command_prompt[j];
			}
			argv[cargc][arglen] = '\0';

			cargc++;
			i += arglen;
		}

		if (strncmp(command_prompt, "arm", 3) == 0) {
			command_function_arm(dataConnection, currentState);
		}

		if (strncmp(command_prompt, "open", 4) == 0) {
			command_function_open(argc, argv, dataConnection);
		}

		if (strncmp(command_prompt, "close", 5) == 0) {
			command_function_close(argc, argv, dataConnection);
		}

		if (strncmp(command_prompt, "cont", 4) == 0) {
			command_function_continue_flight(currentState);
		}

		if (strncmp(command_prompt, "disarm", 6) == 0) {
			command_function_disarm(dataConnection, currentState);
		}

		if (strncmp(command_prompt, "set", 3) == 0) {
			command_function_set(argc, argv, &current_target_label);
		}

		for (int i = 0; i < argc; i++) {
			delete[] argv[i];
		}
		delete[] argv;
		delete[] command_prompt;
		
		is_entering_command = false;
		command_prompt = new char[COLS - 3];
		clear_command_bar(win);

		has_update = true;
		render_main_window(currentState);
	} else if (ch == 127) { // Backspace
		command_prompt[len - 1] = 0;
	} else if (ch >= 0x20 && ch < 0x7F) { // Alpha-numeric and special characters
		command_prompt[len] = (char)(ch & 0xFF);
	}

	render_command_bar(win);
}

static void update_camera_size(WINDOW* win, CurrentState* cs) {
	double width = camera_width(currentState->get_altitude());
	double depth = camera_depth(currentState->get_altitude());
	double area = width * depth;

	refresh();

	init_pair(COLOR_NORMAL_TEXT, COLOR_WHITE, COLOR_BLACK);
	wattrset(win, COLOR_PAIR(COLOR_NORMAL_TEXT));

	mvwprintw(
		win,
		1, 2,
		"Camera size: Width: %3.1f m; Depth: %3.1f m;     ",
		width,
		depth
	);
	mvwprintw(
		win,
		2, 2,
		"             Area: %4.1f m^2;     ",
		area
	);
}

static void update_lat_lng(WINDOW* win, CurrentState* cs) {
	mvwprintw(
		win,
		3, 2,
		"Lat: % 4.6f, Lng: % 4.6f",
		cs->get_latitude(),
		cs->get_longitude()
	);
}

static void update_recording(WINDOW* win, CurrentState* cs) {
	mvwprintw(
		win,
		4, 2,
		"Recording: %d. Current label: %s",
		cs->get_recording(),
		current_target_label == NULL ? "" : current_target_label
	);
}

static void update_continuing_flight(WINDOW* win, CurrentState* cs) {
	mvwprintw(
		win,
		5, 2,
		"Continuing flight: %s",
		cs->get_continuing_flight() ? "YES" : "NO "
	);
}

static void update_armed_status(WINDOW* win, CurrentState* cs) {
	mvwprintw(
		win,
		6, 2,
		"Currently armed: %s",
		cs->get_armed() ? "YES" : "NO "
	);
}

static void update_current_battery(WINDOW* win) {
	mvwprintw(
		win,
		7, 2,
		"Current battery: %s",
		battery_name == NULL ? "" : battery_name
	);
}

static void render_main_window(CurrentState* cs) {
	if (has_update && current_selected_menu == 0) {
		update_camera_size(main_window, cs);
		update_lat_lng(main_window, cs);
		update_recording(main_window, cs);
		update_continuing_flight(main_window, cs);
		update_armed_status(main_window, cs);
		update_current_battery(main_window);
	}

	if (has_update) {
		has_update = false;
		wrefresh(main_window);
		refresh();
	}
}

static void update_cs_displays(CurrentState* cs) {
	has_update = true;
}

static void clear_main_window() {
	wclear(main_window);
	wborder(main_window, '|', '|', '-', '-', '+', '+', '+', '+');
	wrefresh(main_window);
	refresh();
}

static void handle_menu_keypress(int ch) {
	switch (ch) {
		case 'h' :
		case KEY_LEFT :
			if (current_selected_menu > 0) {
				current_selected_menu--;
			}
			clear_main_window();
			has_update = true;
			break;

		case 'l' :
		case KEY_RIGHT :
			if (current_selected_menu < 4) {
				current_selected_menu++;
			}
			clear_main_window();
			has_update = true;
			break;

		case 'j' :
		case KEY_DOWN :
			is_menu_selected = false;
			has_update = true;
			break;

		case 10 /*KEY_ENTER*/ :
			if (current_selected_menu == 4) {
				should_stop = true;
			}
			break;
	}

	render_menu_selector(menu_window);
}

static void handle_main_window_keypress(int ch) {
	
}

int start_gui(int argc, char** argv, CurrentState* cs, std::vector<std::string>* log, Connection* conn, Configuration* conf) {
	dataConnection = conn;
	currentState = cs;
	config = conf;
	clear_flight(&flight);

	setlocale(LC_ALL, "");
	initscr();

	keypad(stdscr, TRUE);
	cbreak();
	noecho();
	start_color();
	curs_set(0);

	refresh();

	menu_window = newwin(3, 0, 0, 0);
	main_window = newwin(LINES - 4, 0, 2, 0);
	command_window = newwin(3, 0, LINES - 3, 0);
	command_prompt = new char[COLS - 3];

	wborder(main_window, '|', '|', '-', '-', '+', '+', '+', '+');
	wborder(command_window, '|', '|', '-', '-', '+', '+', '+', '+');
	wrefresh(main_window);
	wrefresh(command_window);

	render_menu_selector(menu_window);

	currentState->update_callback = &update_cs_displays;
	//currentState->landed_callback = &stop_flying;
	//currentState->flying_callback = &start_flying;
	
	timeout(50);

	while (!should_stop) {
		int ch = getch();
		if (ch == ':') {
			is_entering_command = true;
			render_command_bar(command_window);
		} else if (ch == 27) {	
			if (is_entering_command) {
				delete[] command_prompt;
				command_prompt = new char[COLS - 3];
				clear_command_bar(command_window);
				is_entering_command = false;
				has_update = true;
				render_main_window(cs);
			} else {
				is_menu_selected = true;
				has_update = true;
				render_menu_selector(menu_window);
			}
		} else if (is_entering_command) {
			handle_command_bar_keypress(ch, command_window);
			render_command_bar(command_window);
		} else if (is_menu_selected) {
			handle_menu_keypress(ch);
		} else if (current_selected_menu == MENU_MAIN) {
			handle_main_window_keypress(ch);
		}

		render_main_window(cs);
	}

	endwin();

	return 0;
}
