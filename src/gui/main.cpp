#include <gtk/gtk.h>
#include <cstring>
#include <string>
#include <vector>
#include "../data/CurrentState.h"
#include "../connection/Connection.h"
#include <gdk/gdk.h>

static Connection* dataConnection;
static CurrentState* currentState;
static GtkBuilder* builder;

static void toggle_recording(GtkWidget* widget, gpointer data) {
	GtkButton* button = (GtkButton*)widget;

	const char* currentLabel = gtk_button_get_label(button);
	const gchar* startLabel = "Start Recording";
	const gchar* stopLabel = "Stop Recording";

	if (memcmp(currentLabel, startLabel, 3)) {
		gtk_button_set_label(button, startLabel);
	} else {
		gtk_button_set_label(button, stopLabel);
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

static void toggle_arm_button_click() {
	dataConnection->toggle_arm();
}

static void current_state_update(CurrentState* cs) {
	{
		char coordinates_description[16];
		sprintf(coordinates_description, "%2.6f, %2.6f", cs->get_latitude(), cs->get_longitude());
		GtkLabel* coordinates_label = (GtkLabel*)gtk_builder_get_object(builder, "currentCoordinates");
		gtk_label_set_text(coordinates_label, coordinates_description);
	}

	{
		const gchar* armedLabel = "ARMED";
		const gchar* unarmedLabel = "UNARMED";
		bool is_armed = cs->get_armed();
		GtkLabel* armed_label = (GtkLabel*)gtk_builder_get_object(builder, "armedStatus");
		gtk_label_set_text(armed_label, is_armed ? armedLabel : unarmedLabel);
	}
}

int start_gui(int argc, char** argv, CurrentState* cs, std::vector<std::string>* log, Connection* conn) {
	dataConnection = conn;
	currentState = cs;

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

	currentState->update_callback = &current_state_update;

	gtk_widget_show_all((GtkWidget*)window);

	gtk_main();

	return 0;
}
