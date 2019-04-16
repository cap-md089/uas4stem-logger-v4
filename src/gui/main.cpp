#include <gtk/gtk.h>
#include <cstring>
#include <string>
#include <vector>
#include "../data/CurrentState.h"
#include "../connection/Connection.h"
#include <gdk/gdk.h>
#include <iostream>
#include "../config/config.h"
#include <math.h>

static Connection* dataConnection;
static CurrentState* currentState;
static Configuration* config;
static GtkBuilder* builder;

static std::vector<double> voltages;

static void toggle_recording(GtkWidget* widget, gpointer data) {
	GtkButton* button = (GtkButton*)widget;
	GtkLabel* recorded_coordinates_label = (GtkLabel*)gtk_builder_get_object(builder, "recordedCoordinates");
	GtkEntry* current_target = (GtkEntry*)gtk_builder_get_object(builder, "targetDescription");

	const gchar* startLabel = "Start Recording";
	const gchar* stopLabel = "Stop Recording";
	const gchar* recordingMsg = "Recording...";

	if (currentState->get_recording()) {
	//if (currentLabel[2] != startLabel[2]) {
	//if (memcmp(currentLabel, startLabel, 3)) {
		gtk_button_set_label(button, startLabel);
		RecordedCoordinates* recorded = currentState->stop_recording();
	
		char coordinates_description[24];
		sprintf(coordinates_description, "%2.6f, %2.6f", recorded->latitude, recorded->longitude);

		gtk_label_set_text(recorded_coordinates_label, coordinates_description);
	} else {
		GtkComboBox* targetX = (GtkComboBox*)gtk_builder_get_object(builder, "targetPositionX");
		GtkComboBox* targetY = (GtkComboBox*)gtk_builder_get_object(builder, "targetPositionY");
		float targetXSelected, targetYSelected;
		
		targetXSelected = gtk_combo_box_get_active(targetX) - 4.0f;
		targetYSelected = gtk_combo_box_get_active(targetY) - 3.0f;

		gtk_button_set_label(button, stopLabel);
		currentState->start_recording(targetXSelected / 4, targetYSelected / 3);
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

static void continue_flight() {
	currentState->continue_flight();

	const gchar* continuing_flight_label = "Continuing flight: YES";

	GtkLabel* label = (GtkLabel*)gtk_builder_get_object(builder, "flightContinueStatus");
	gtk_label_set_text(label, continuing_flight_label);
}

static void close_golf_button_click() {
	dataConnection->close_golf_bottle();
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

static bool checking_voltage = false;

static int update_voltage_display(gpointer box) {
	GtkTextView* voltage_box = (GtkTextView*)box;

	if (currentState->get_flying()) {
		char newtext[10];
		sprintf(newtext, "%2.4f\n", currentState->get_battery_voltage());

		GtkTextIter start_position, end_position;
		GtkTextBuffer* buffer = gtk_text_view_get_buffer(voltage_box);
		gtk_text_buffer_get_end_iter(buffer, &end_position);
		gtk_text_buffer_get_start_iter(buffer, &start_position);
		char* old_string = (char*)gtk_text_buffer_get_text(buffer, (const GtkTextIter*)&start_position, (const GtkTextIter*)&end_position, true);
		strcat(old_string, newtext);
		gtk_text_buffer_set_text(buffer, old_string, -1);

		return G_SOURCE_CONTINUE;
	} else {
		checking_voltage = false;
		return G_SOURCE_REMOVE;
	}
}

static int update_packets_per_second(gpointer lbl) {
	GtkLabel* pps_label = (GtkLabel*)lbl;

	char packets_per_second_text[10];
	sprintf(packets_per_second_text, "%6.2f", dataConnection->get_packets_per_second());
	gtk_label_set_text(pps_label, packets_per_second_text);

	return G_SOURCE_CONTINUE;
}

static int update_coordinates_label(gpointer lbl) {
	GtkLabel* coordinates_label = (GtkLabel*)lbl;

	char coordinates_description[24];
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

	char time_description_text[66]; 
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

static void current_state_update(CurrentState* cs) {
	GObject* obj;

	obj = gtk_builder_get_object(builder, "armedStatus");
	g_main_context_invoke(NULL, update_armed_label, obj);

	obj = gtk_builder_get_object(builder, "currentCoordinates");
	g_main_context_invoke(NULL, update_coordinates_label, obj);

	if (!checking_voltage && cs->get_flying()) {
		obj = gtk_builder_get_object(builder, "voltageData");
		update_voltage_display((gpointer)obj);
		g_timeout_add(10000, G_SOURCE_FUNC(update_voltage_display), obj);

		checking_voltage = true;
	}

	obj = gtk_builder_get_object(builder, "timeDescriptions");
	g_main_context_invoke(NULL, update_time_descriptions, obj);
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
}

int start_gui(int argc, char** argv, CurrentState* cs, std::vector<std::string>* log, Connection* conn, Configuration* conf) {
	dataConnection = conn;
	currentState = cs;
	config = conf;

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

	currentState->update_callback = &current_state_update;
	currentState->landed_callback = &stop_flying;

	gtk_widget_show_all((GtkWidget*)window);

	gtk_main();

	free(dataConnection);
	free(currentState);
	free(config);
	free(builder);

	return 0;
}
