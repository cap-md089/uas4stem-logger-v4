#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "../data/CurrentState.h"
#include <thread>
#include <winsock.h>
#include <vector>
#include <string>
#include <chrono>

class Connection {
	public:
		/**
		 * Commands that can be sent to the Python client
		 */
		void toggle_arm();
		void open_charlie_bottle();
		void close_charlie_bottle();
		void open_golf_bottle();
		void close_golf_bottle();

		/**
		 * Nice for the operator to know how good the connection is
		 */
		double get_packets_per_second();

		/**
		 * Controls from the main thread to start and stop the connection client and server
		 */
		void setup(CurrentState* cs, std::vector<std::string>* log);
		void stop();
	private:
		SOCKET udp_data_socket;
		SOCKET tcp_command_socket;

		void server_thread(CurrentState* cs, std::vector<std::string>* log);

		void send_shutdown();

		std::thread udp_thread;

		unsigned int packet_count;
		std::chrono::system_clock::time_point last_packet_clear;

		bool should_stop;
};

#endif
