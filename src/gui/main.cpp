#include <gtk/gtk.h>
#include <cstring>
#include <string>
#include <vector>
#include "../data/CurrentState.h"
#include "../data/Session.h"
#include "../connection/Connection.h"
#include <gdk/gdk.h>
#include <iostream>
#include "../config/config.h"
#include <time.h>
#include <math.h>
#include <fstream>

static Connection* dataConnection;
static CurrentState* currentState;
static Configuration* config;

static CurrentFlight flight;
static CurrentFlight overall_flight;

static GtkBuilder* builder;

static int flight_count = 0;
static int timeout_id;

static std::vector<bool> selected_coordinates;

typedef struct CoordinateGroup {
	uint64_t id;
	GtkButton* select_button;
	GtkButton* deselect_button;
} CoordinateGroup;

static double camera_width(double alt) {
	return 2 * 1.04891304331  * alt;
}

static double camera_depth(double alt) {
	return 2 * 0.489130434783 * alt;
}

static void deselect_coordinate(gpointer, gpointer);
static void select_coordinate(gpointer, gpointer);

static void add_possible_coordinate(RecordedTarget* target) {
	uint64_t id = selected_coordinates.size();
	selected_coordinates.push_back(false);

	GtkBox* new_box			= (GtkBox*)gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkBox* available_box	= (GtkBox*)gtk_builder_get_object(builder, "possibleCoordinates");

	GtkLabel*	desc_label		= (GtkLabel*)	gtk_label_new((target->description).data());
	GtkButton*	select_button	= (GtkButton*)	gtk_button_new_with_label("Select");	
	GtkButton*	deselect_button	= (GtkButton*)	gtk_button_new_with_label("Deselect");

	gtk_box_pack_start(new_box, (GtkWidget*)desc_label,			false, false, 10);
	gtk_box_pack_start(new_box, (GtkWidget*)select_button,		false, false, 10);
	gtk_box_pack_start(new_box, (GtkWidget*)deselect_button,	false, false, 10);

	gtk_box_pack_start(available_box, (GtkWidget*)new_box, false, false, 10);

	CoordinateGroup* coordinate_group = new CoordinateGroup;
	coordinate_group->id = id;
	coordinate_group->deselect_button = deselect_button;
	coordinate_group->select_button = select_button;

	gtk_widget_set_sensitive((GtkWidget*)deselect_button, FALSE);

	g_signal_connect((GObject*)select_button,	"clicked", G_CALLBACK(select_coordinate),	coordinate_group);
	g_signal_connect((GObject*)deselect_button,	"clicked", G_CALLBACK(deselect_coordinate),	coordinate_group);

	gtk_widget_show_all((GtkWidget*)new_box);
}

static void select_coordinate(gpointer box, gpointer data_pointer) {
	CoordinateGroup* data = (CoordinateGroup*)data_pointer;

	uint64_t id = data->id;
	GtkButton* select_button = data->select_button;
	GtkButton* deselect_button = data->deselect_button;

	selected_coordinates[id] = true;

	gtk_widget_set_sensitive((GtkWidget*)select_button,		FALSE);
	gtk_widget_set_sensitive((GtkWidget*)deselect_button,	TRUE);
}

static void deselect_coordinate(gpointer fakeid, gpointer data_pointer) {
	CoordinateGroup* data = (CoordinateGroup*)data_pointer;

	uint64_t id = data->id;
	GtkButton* select_button = data->select_button;
	GtkButton* deselect_button = data->deselect_button;

	selected_coordinates[id] = false;

	gtk_widget_set_sensitive((GtkWidget*)select_button,		TRUE);
	gtk_widget_set_sensitive((GtkWidget*)deselect_button,	FALSE);
}

static void generate_flight_plan() {
	int count = 0;
	for (unsigned int i = 0; i < selected_coordinates.size(); i++) {
		if (selected_coordinates[i]) {
			count++;
		}
	}
	if (!count) {
		return;
	}

	int flight_count = count / 2;
	// 108 per command; 3 commands for each drop, 3 commands for each flight, 1 or 2 drops per flight
	int file_size = 13 + 56 + (108 * count * 3) + (108 * (flight_count + 1) * 3);
	int current_byte = 0;
	int command_number = 0;

	char* file_text = (char*)malloc(sizeof(char) * file_size);
	for (unsigned int i = 0; i < (sizeof(char) * file_size); i++) {
		*(file_text + i) = 0;
	}

	sprintf(
		file_text,
		"QGC WPL 110\r\n%d 1 0 16 0 0 0 0 %.8f %.8f 16.440001 1\r\n",
		command_number++,
		currentState->get_latitude(),
		currentState->get_longitude()
	);

	// Go until NULL
	while (*(file_text + current_byte)) {
		current_byte++;
	}

	// 0 for charlie, 1 for golf
	int current_bottle = 0;

	for (unsigned int i = 0; i < selected_coordinates.size(); i++) {
		if (!selected_coordinates[i]) {
			continue;
		}

		double lat, lng;
		RecordedTarget* target = overall_flight.targets.at(i);
		lat = target->latitude;
		lng = target->longitude;

		if (current_bottle == 0) {
			// Takeoff command
			sprintf(
				file_text + current_byte,
				"%d 0 3 22 0 0 0 0 0 0 20.0 1\r\n",
				command_number
			);
			command_number++;

			// Go until NULL
			while (*(file_text + current_byte)) {
				current_byte++;
			}

			sprintf(
				file_text + current_byte,
				"%d 0 3 16 0 0 0 0 %.8f %.8f 20.0 1\r\n",
				command_number,
				currentState->get_latitude(),
				currentState->get_longitude()
			);
			command_number++;

			// Go until NULL
			while (*(file_text + current_byte)) {
				current_byte++;
			}
		}

		// Go to target and open servo
		sprintf(
			file_text + current_byte,
			"%d 0 3 16 0 0 0 0 %.8f %.8f 15 1\r\n"
			"%d 0 3 183 %d 1000 0 0 0 0 1\r\n"
			"%d 0 3 16 5 0 0 0 %.8f %.8f 15 1\r\n",
			command_number,
			lat,
			lng,
			command_number + 1,
			5 + current_bottle,
			command_number + 2,
			lat,
			lng
		);

		while (*(file_text + current_byte)) {
			current_byte++;
		}

		command_number += 3;

		if (current_bottle == 1 || (i == (selected_coordinates.size() - 1))) {
			sprintf(
				file_text + current_byte,
				"%d 0 3 20 0 0 0 0 0 0 0 1\r\n",
				command_number++
			);

			// Go until NULL
			while (*(file_text + current_byte)) {
				current_byte++;
			}
		}

		current_bottle = (current_bottle + 1) % 2;
	}

	char location[260];
	config->get_data_dir_location(location);
	char file_name[40];
	std::string file_name2;

	time_t rawtime;
	struct tm* time_info;
	time(&rawtime);
	time_info = localtime(&rawtime);
	strftime(file_name, 40, "Plan %Y-%m-%d %H-%M", time_info);

	strcat(location, "\\");
	strcat(location, file_name);
	strcat(location, ".waypoints");
	file_name2 = location;

	std::ofstream fs;
	fs.open(location, std::ios::out | std::ios::binary);
	fs << file_text;
	fs.close();

	dataConnection->send_open_waypoints(file_name2);
	
	free(file_text);
}

static void toggle_recording(GtkWidget* widget, gpointer data) {
	GtkButton* button = (GtkButton*)widget;
	GtkLabel* recorded_coordinates_label = (GtkLabel*)gtk_builder_get_object(builder, "recordedCoordinates");
	GtkEntry* current_target = (GtkEntry*)gtk_builder_get_object(builder, "targetDescription");

	const gchar* startLabel = "Start Recording";
	const gchar* stopLabel = "Stop Recording";
	const gchar* recordingMsg = "Recording...";
	const char* entry_text;

	if (currentState->get_recording()) {
	//if (currentLabel[2] != startLabel[2]) {
	//if (memcmp(currentLabel, startLabel, 3)) {
		gtk_button_set_label(button, startLabel);
		RecordedCoordinates* recorded = currentState->stop_recording();

		entry_text = gtk_entry_get_text(current_target);
		std::string target_description = std::string(entry_text);

		RecordedTarget* target = new RecordedTarget;

		if (recorded->latitude == 0) {
			// Occurs if it didn't record, due to not flying
			recorded->latitude = currentState->get_latitude();
			recorded->longitude = currentState->get_longitude();
		}

		target->latitude = recorded->latitude;
		target->longitude = recorded->longitude;
		target->time_in_air = currentState->get_time_in_air();
		target->description = target_description;

		flight.targets.push_back(target);
		overall_flight.targets.push_back(target);

		add_possible_coordinate(target);

		char coordinates_description[48];
		sprintf(coordinates_description, "%2.6f, %2.6f", recorded->latitude, recorded->longitude);
		gtk_label_set_text(recorded_coordinates_label, coordinates_description);
	} else {
		GtkComboBox* target_left = (GtkComboBox*)gtk_builder_get_object(builder, "targetPositionLeft");
		GtkComboBox* target_forward = (GtkComboBox*)gtk_builder_get_object(builder, "targetPositionForward");
		float target_left_selected, target_forward_selected;
		
		target_left_selected = gtk_combo_box_get_active(target_left) - 4.0f;
		target_forward_selected = gtk_combo_box_get_active(target_forward) - 3.0f;

		gtk_button_set_label(button, stopLabel);
		currentState->start_recording(target_left_selected / -4, target_forward_selected / 3);
		gtk_label_set_text(recorded_coordinates_label, recordingMsg);
	}
}

static void open_charlie_button_click() {
	dataConnection->open_charlie_bottle();
}

static void close_charlie_button_click() {
	dataConnection->close_charlie_bottle();
}

static void open_golf_button_click() {
	dataConnection->open_golf_bottle();
}

static void close_golf_button_click() {
	dataConnection->close_golf_bottle();
}

static void continue_flight() {
	currentState->continue_flight();

	const gchar* continuing_flight_label = "Continuing flight: YES";

	GtkLabel* label = (GtkLabel*)gtk_builder_get_object(builder, "flightContinueStatus");
	gtk_label_set_text(label, continuing_flight_label);
}

static void toggle_arm_button_click() {
	dataConnection->toggle_arm();
}

static void enable_rtl() {
	dataConnection->send_rtl();
}

static void enable_auto() {
	dataConnection->send_auto();
}

static void append_to_gtk_text_view(GtkTextView* textview, const char* text) {
	std::string newtext_string = text;
	std::string oldtext_string;

	GtkTextIter start_position, end_position;
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(textview);
	gtk_text_buffer_get_end_iter(buffer, &end_position);
	gtk_text_buffer_get_start_iter(buffer, &start_position);
	char* oldtext = (char*)gtk_text_buffer_get_text(buffer, (const GtkTextIter*)&start_position, (const GtkTextIter*)&end_position, true);
	oldtext_string = oldtext;
	std::string text_to_set = oldtext_string + newtext_string;
	gtk_text_buffer_set_text(buffer, text_to_set.data(), -1);
}

static bool checking_voltage = false;

static int update_voltage_display(gpointer box) {
	GtkTextView* voltage_box = (GtkTextView*)box;

	checking_voltage = true;

	if (currentState->get_flying()) {
		flight.battery_voltages.push_back(currentState->get_battery_voltage());
		char newtext[20];
		sprintf(newtext, "%2.3f\n", currentState->get_battery_voltage());
		append_to_gtk_text_view(voltage_box, newtext);
		return G_SOURCE_CONTINUE;
	} else {
		checking_voltage = false;
		return G_SOURCE_REMOVE;
	}
}

static int update_packets_per_second(gpointer lbl) {
	GtkLabel* pps_label = (GtkLabel*)lbl;

	char packets_per_second_text[20];
	sprintf(packets_per_second_text, "%6.2f", dataConnection->get_packets_per_second());
	gtk_label_set_text(pps_label, packets_per_second_text);

	return G_SOURCE_CONTINUE;
}

static int update_coordinates_label(gpointer lbl) {
	GtkLabel* coordinates_label = (GtkLabel*)lbl;

	char coordinates_description[48];
	sprintf(coordinates_description, "%2.6f, %2.6f", currentState->get_latitude(), currentState->get_longitude());
	gtk_label_set_text(coordinates_label, coordinates_description);

	return G_SOURCE_REMOVE;
}

static int update_armed_label(gpointer lbl) {
	GtkLabel* armed_label = (GtkLabel*)lbl;

	const gchar* armedLabel = "ARMED";
	const gchar* unarmedLabel = "UNARMED";
	bool is_armed = currentState->get_armed();
	gtk_label_set_text(armed_label, is_armed ? armedLabel : unarmedLabel);

	return G_SOURCE_REMOVE;
}

static int update_time_descriptions(gpointer lbl) {
	GtkTextView* time_descriptions_box = (GtkTextView*)lbl;

	char time_description_text[74]; 
	unsigned int time_in_air = currentState->get_battery_timer();
	unsigned int max_time = config->get_max_flight_time();
	unsigned int time_left = max_time - time_in_air;
	unsigned int time_required = roundf(currentState->get_time_required_for_landing());

	sprintf(
		time_description_text,
		"Time in air: %02u:%02u\nTime left: %02u:%02u\nTime required to land: %02u:%02u",
		time_in_air / 60, time_in_air % 60,
		time_left / 60, time_left % 60,
		time_required / 60, time_required % 60
	);

	GtkTextBuffer* buffer = gtk_text_view_get_buffer(time_descriptions_box);
	gtk_text_buffer_set_text(buffer, time_description_text, -1);

	return G_SOURCE_REMOVE;
}

static int update_camera_size(gpointer lbl) {
	GtkLabel* camera_size_desc = (GtkLabel*)lbl;

	double width = camera_width(currentState->get_altitude());
	double depth = camera_depth(currentState->get_altitude());
	double area = width * depth;
	char camera_size_desc_text[250];

	sprintf(
		camera_size_desc_text,
		"Camera size: Width: %3.1f m; Depth: %3.1f m; Area: %4.1f m;",
		width,
		depth,
		area
	);

	gtk_label_set_text(camera_size_desc, camera_size_desc_text);

	return G_SOURCE_REMOVE;
}

static void current_state_update(CurrentState* cs) {
	GObject* obj;

	obj = gtk_builder_get_object(builder, "armedStatus");
	g_main_context_invoke(NULL, update_armed_label, obj);

	obj = gtk_builder_get_object(builder, "currentCoordinates");
	g_main_context_invoke(NULL, update_coordinates_label, obj);

	obj = gtk_builder_get_object(builder, "timeDescriptions");
	g_main_context_invoke(NULL, update_time_descriptions, obj);

	obj = gtk_builder_get_object(builder, "cameraSize");
	g_main_context_invoke(NULL, update_camera_size, obj);
}

static int end_voltage_display(gpointer obj) {
	obj = gtk_builder_get_object(builder, "voltageData");

	char stopped[28];
	sprintf(stopped, "--- Flight %d stopped ---\n", flight_count);
	append_to_gtk_text_view((GtkTextView*)obj, stopped);
	g_source_remove(timeout_id);
	timeout_id = 0;

	return G_SOURCE_REMOVE;
}

static int start_voltage_display_update(gpointer obj) {
	obj = gtk_builder_get_object(builder, "voltageData");

	char started[28];
	sprintf(started, "--- Flight %d started ---\n", flight_count);
	append_to_gtk_text_view((GtkTextView*)obj, started);

	update_voltage_display((gpointer)obj);

	timeout_id = g_timeout_add(10000, G_SOURCE_FUNC(update_voltage_display), obj);

	return G_SOURCE_REMOVE;
}

static void start_flying(CurrentState* cs) {
	flight_count++;

	g_main_context_invoke(NULL, start_voltage_display_update, NULL);
}

static int stop_flying_label(gpointer lbl) {
	GtkLabel* label = (GtkLabel*)lbl;

	const gchar* stop_flying_label = "Continuing flight: NO";

	gtk_label_set_text(label, stop_flying_label);

	return G_SOURCE_REMOVE;
}

static void stop_flying(CurrentState* cs, bool continued) {
	GObject* obj;

	obj = gtk_builder_get_object(builder, "flightContinueStatus");
	g_main_context_invoke(NULL, stop_flying_label, obj);

	obj = gtk_builder_get_object(builder, "voltageData");
	g_main_context_invoke(NULL, end_voltage_display, obj);

	flight.battery_voltages.push_back(cs->get_battery_voltage());
	obj = gtk_builder_get_object(builder, "batterySelectID");
	save_flight(
		config,
		&flight,
		cs->get_battery_timer(),
		gtk_combo_box_get_active((GtkComboBox*)obj)
	);

	clear_flight(&flight);
}

static void update_altitude_calculations_configuration(gpointer entry) {
	GtkEntry* altitude_entry = (GtkEntry*)entry;
	GtkLabel* camera_size_desc = (GtkLabel*)gtk_builder_get_object(builder, "altitudeCheckOutput");

	const char* altitude_entered = gtk_entry_get_text(altitude_entry);

	double altitude;
	sscanf(altitude_entered, "%lf", &altitude);

	double width = camera_width(altitude);
	double depth = camera_depth(altitude);
	double area = width * depth;
	char camera_size_desc_text[250];

	sprintf(
		camera_size_desc_text,
		"Camera size: Width: %3.1f m; Depth: %3.1f m; Area: %4.1f m;",
		width,
		depth,
		area
	);

	gtk_label_set_text(camera_size_desc, camera_size_desc_text);
}

static void update_max_flight_time(gpointer entry) {
	GtkEntry* max_flight_time_entry = (GtkEntry*)entry;

	const char* max_flight_entered = gtk_entry_get_text(max_flight_time_entry);

	unsigned int time;
	sscanf(max_flight_entered, "%ud", &time);

	config->set_max_flight_time(time);
	config->save();
}

static void update_scrabble_box(gpointer s_entry) {
	GtkEntry* scrabble_entry = (GtkEntry*)s_entry;
	GtkTextView* scrabble_box = (GtkTextView*)gtk_builder_get_object(builder, "scrabbleOutput");

	const char* char_text = gtk_entry_get_text(scrabble_entry);
	std::string text = char_text;

	if (text.size() < 3) {
		return;
	}

	std::ifstream file;
	file.open("englishwords.txt", std::ifstream::in | std::ifstream::ate);

	if (!file.good()) {
		return;
	}
	
	uint64_t size = file.tellg();

	char* buffer = new char[size];

	file.read(buffer, size);

	for (uint64_t i = 0; i < size; i++) {
		bool is_valid = false;
		uint8_t j = 0;

		while ((*(buffer + i + j) != '\r') && !is_valid) {
			for (unsigned char k = 0; (k < text.size()) && !is_valid; k++) {
				if (text[k] == *(buffer + i + j)) {
					is_valid = true;
				}
			}
			j++;
		}

		if (is_valid) {
			char* text = new char[j + 2];
			std::memcpy(text, buffer + i, j);
			text[j - 1] = '\n';
			text[j] = '\0';
			append_to_gtk_text_view(scrabble_box, text);

			free(text);
		}

		j++;
	}

	free(buffer);
}

int start_gui(int argc, char** argv, CurrentState* cs, std::vector<std::string>* log, Connection* conn, Configuration* conf) {
	dataConnection = conn;
	currentState = cs;
	config = conf;
	clear_flight(&flight);

	GObject* window;
	GObject* obj;
	GError* error = NULL;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	if (gtk_builder_add_from_file(builder, "layout.glade", &error) == 0) {
		g_printerr("Error loading from file: %s\n", error->message);
		g_clear_error(&error);
		return 1;
	}

	window = gtk_builder_get_object(builder, "window");
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	obj = gtk_builder_get_object(builder, "startStopRecording");
	g_signal_connect(obj, "clicked", G_CALLBACK(toggle_recording), NULL);

	obj = gtk_builder_get_object(builder, "openCharlie");
	g_signal_connect(obj, "clicked", G_CALLBACK(open_charlie_button_click), NULL);

	obj = gtk_builder_get_object(builder, "openGolf");
	g_signal_connect(obj, "clicked", G_CALLBACK(open_golf_button_click), NULL);

	obj = gtk_builder_get_object(builder, "closeCharlie");
	g_signal_connect(obj, "clicked", G_CALLBACK(close_charlie_button_click), NULL);

	obj = gtk_builder_get_object(builder, "closeGolf");
	g_signal_connect(obj, "clicked", G_CALLBACK(close_golf_button_click), NULL);

	obj = gtk_builder_get_object(builder, "toggleArmButton");
	g_signal_connect(obj, "clicked", G_CALLBACK(toggle_arm_button_click), NULL);

	obj = gtk_builder_get_object(builder, "connectionSpeedDisplay");
	g_timeout_add(500, G_SOURCE_FUNC(update_packets_per_second), obj);

	obj = gtk_builder_get_object(builder, "sendrtl");
	g_signal_connect(obj, "clicked", G_CALLBACK(enable_rtl), NULL);

	obj = gtk_builder_get_object(builder, "sendauto");
	g_signal_connect(obj, "clicked", G_CALLBACK(enable_auto), NULL);

	obj = gtk_builder_get_object(builder, "continueFlight");
	g_signal_connect(obj, "clicked", G_CALLBACK(continue_flight), NULL);

	obj = gtk_builder_get_object(builder, "altitudeCheckEntry");
	g_signal_connect(obj, "changed", G_CALLBACK(update_altitude_calculations_configuration), obj);

	obj = gtk_builder_get_object(builder, "maxFlightTimeEntry");
	g_signal_connect(obj, "changed", G_CALLBACK(update_max_flight_time), obj);
	{
		char entry_text[5];
		sprintf(entry_text, "%d", conf->get_max_flight_time());
		gtk_entry_set_text((GtkEntry*)obj, entry_text);
	}

	obj = gtk_builder_get_object(builder, "generateFlightPlan");
	g_signal_connect(obj, "clicked", G_CALLBACK(generate_flight_plan), NULL);

	obj = gtk_builder_get_object(builder, "scrabbleInputsEntry");
	g_signal_connect(obj, "changed", G_CALLBACK(update_scrabble_box), NULL);

	currentState->update_callback = &current_state_update;
	currentState->landed_callback = &stop_flying;
	currentState->flying_callback = &start_flying;

	gtk_widget_show_all((GtkWidget*)window);

	gtk_main();

	free(dataConnection);
	free(currentState);
	free(config);
	free(builder);

	return 0;
}
