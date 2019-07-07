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
typedef int SOCKET;
#endif

#define COMMAND_PORT 1337
#define DATA_PORT 54248

#define CAN_SEND_COMMANDS 1


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
		void send_open_waypoints(std::string);

		/**
		 * Nice for the operator to know how good the connection is
		 */
		double get_packets_per_second();

		/**
		 * Controls from the main thread to start and stop the connection client and server
		 */
		void setup(CurrentState* cs, std::vector<std::string>* log);
		void stop();

		//void python_client_connect();
		void tcp_server_listen();
		void udp_server_listen(CurrentState* cs);
	private:
		bool tcp_is_open;

		SOCKET udp_data_socket;
		std::vector<SOCKET> tcp_command_sockets;
		SOCKET tcp_server;

		void send_shutdown();

		std::thread udp_thread;
		std::thread tcp_thread;

		unsigned int packet_count;
		std::chrono::system_clock::time_point last_packet_clear;

		bool should_stop;
};

#endif
