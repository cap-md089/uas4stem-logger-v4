#include "../data/CurrentState.h"
#include "../data/Session.h"
#include "../connection/Connection.h"
#include "../config/config.h"
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

static WINDOW* menu_window;
static WINDOW* main_window;

static Connection* dataConnection;
static CurrentState* currentState;
static Configuration* config;
static CurrentFlight flight;
static CurrentFlight overall_flight;

static int flight_count = 0;
static int timeout_id;

static std::vector<bool> selected_coordinates;

static int current_selected_menu = 0;
static bool is_menu_selected = true;
static bool has_update = true;

static double camera_width(double alt) {
	return 2 * 1.04891304331  * alt;
}

static double camera_depth(double alt) {
	return 2 * 0.489130434783 * alt;
}

void render_menu_selector(WINDOW* win) {
	wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');

	init_pair(COLOR_SELECTED_MENU_ITEM, COLOR_BLACK, COLOR_WHITE);
	init_pair(COLOR_UNSELECTED_MENU_ITEM, COLOR_WHITE, COLOR_BLACK);

	wattrset(win, COLOR_PAIR(COLOR_SELECTED_MENU_ITEM));

	if (current_selected_menu == 0) {
		mvwaddstr(win, 1, 2, "Data");
		wattrset(win, COLOR_PAIR(COLOR_UNSELECTED_MENU_ITEM));
		mvwaddstr(win, 1, 7, "Conf");
		mvwaddstr(win, 1, 12, "Coord");
		mvwaddstr(win, 1, 18, "Scrabble");
		mvwaddstr(win, 1, 27, "Quit");
	} else if (current_selected_menu == 1) {
		mvwaddstr(win, 1, 7, "Conf");
		wattrset(win, COLOR_PAIR(COLOR_UNSELECTED_MENU_ITEM));
		mvwaddstr(win, 1, 2, "Data");
		mvwaddstr(win, 1, 12, "Coord");
		mvwaddstr(win, 1, 18, "Scrabble");
		mvwaddstr(win, 1, 27, "Quit");
	} else if (current_selected_menu == 2) {
		mvwaddstr(win, 1, 12, "Coord");
		wattrset(win, COLOR_PAIR(COLOR_UNSELECTED_MENU_ITEM));
		mvwaddstr(win, 1, 7, "Conf");
		mvwaddstr(win, 1, 2, "Data");
		mvwaddstr(win, 1, 18, "Scrabble");
		mvwaddstr(win, 1, 27, "Quit");
	} else if (current_selected_menu == 3) {
		mvwaddstr(win, 1, 18, "Scrabble");
		wattrset(win, COLOR_PAIR(COLOR_UNSELECTED_MENU_ITEM));
		mvwaddstr(win, 1, 7, "Conf");
		mvwaddstr(win, 1, 2, "Data");
		mvwaddstr(win, 1, 12, "Coord");
		mvwaddstr(win, 1, 27, "Quit");
	} else if (current_selected_menu == 4) {
		mvwaddstr(win, 1, 27, "Quit");
		wattrset(win, COLOR_PAIR(COLOR_UNSELECTED_MENU_ITEM));
		mvwaddstr(win, 1, 7, "Conf");
		mvwaddstr(win, 1, 2, "Data");
		mvwaddstr(win, 1, 12, "Coord");
		mvwaddstr(win, 1, 18, "Scrabble");
	}

	wrefresh(win);
}

void render_command_bar() {

}

static void update_camera_size(WINDOW* win) {
	double width = camera_width(currentState->get_altitude());
	double depth = camera_depth(currentState->get_altitude());
	double area = width * depth;

	refresh();

	init_pair(COLOR_NORMAL_TEXT, COLOR_WHITE, COLOR_BLACK);
	wattrset(win, COLOR_PAIR(COLOR_NORMAL_TEXT));

	mvwprintw(
		win,
		1, 2,
		"Camera size: Width: %3.1f m; Depth: %3.1f m;",
		width,
		depth
	);
	mvwprintw(
		win,
		2, 2,
		"             Area: %4.1f m^2;",
		area
	);

	wrefresh(win);
	wrefresh(main_window);
	refresh();
}

static void update_cs_displays_main() {
	if (has_update) {
		has_update = false;
		update_camera_size(main_window);
	}
}

static void update_cs_displays(CurrentState* cs) {
	has_update = true;
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

	refresh();

	menu_window = newwin(3, 0, 0, 0);
	main_window = newwin(LINES - 2, 0, 2, 0);

	wborder(main_window, '|', '|', '-', '-', '+', '+', '+', '+');
	wrefresh(main_window);

	render_menu_selector(menu_window);

	currentState->update_callback = &update_cs_displays;
	//currentState->landed_callback = &stop_flying;
	//currentState->flying_callback = &start_flying;
	
	timeout(250);

	bool should_stop = false;
	while (!should_stop) {
		int ch = getch();
		switch (ch) {
			case 'q' :
				should_stop = true;
			break;

			case 'l' :
				if (is_menu_selected) {
					if (current_selected_menu < 5) {
						current_selected_menu++;
						render_menu_selector(menu_window);
					}
				}
			break;

			case 'h' :
				if (is_menu_selected) {
					if (current_selected_menu > 0) {
						current_selected_menu--;
						render_menu_selector(menu_window);
					}
				}
			break;
		}
		update_cs_displays_main();
	}

	endwin();

	return 0;
}
