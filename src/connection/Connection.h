#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "../data/CurrentState.h"
#include <thread>
#include <vector>
#include <string>
#include <chrono>

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

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
		void send_rtl();
		void send_auto();

		/**
		 * Nice for the operator to know how good the connection is
		 */
		double get_packets_per_second();

		/**
		 * Controls from the main thread to start and stop the connection client and server
		 */
		void setup(CurrentState* cs, std::vector<std::string>* log);
		void stop();

		void python_client_connect();
		void udp_server_listen(CurrentState* cs);
	private:
		bool tcp_is_open;

#ifdef _WIN32
		SOCKET udp_data_socket;
		SOCKET tcp_command_socket;
#else
		socket udp_data_socket;
		socket tcp_command_socket;
#endif

		void send_shutdown();

		std::thread udp_thread;
		std::thread tcp_thread;

		unsigned int packet_count;
		std::chrono::system_clock::time_point last_packet_clear;

		bool should_stop;
};

#endif
