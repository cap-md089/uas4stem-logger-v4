#include "Connection.h"
#include "../data/CurrentState.h"
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <stdio.h>
#include <iostream>

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

#define CLIENT_PORT 1337
#define SERVER_PORT 54248

#define CAN_SEND_COMMANDS 1

const char header[5] = {
	'C', 'S', 'D', 'A', 'T'
};

void Connection::open_charlie_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 5, 1 };
	if (tcp_is_open) {
		send(tcp_command_socket, command, 4, 0);
	}
#endif
}

void Connection::close_charlie_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 5, 2 };
	if (tcp_is_open) {
		send(tcp_command_socket, command, 4, 0);
	}
#endif
}

void Connection::open_golf_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 6, 1 };
	if (tcp_is_open) {
		send(tcp_command_socket, command, 4, 0);
	}
#endif
}

void Connection::close_golf_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 6, 2 };
	if (tcp_is_open) {
		send(tcp_command_socket, command, 4, 0);
	}
#endif
}

void Connection::toggle_arm() {
#if CAN_SEND_COMMANDS == 1
	char command[2] = { 1, 2 };
	if (tcp_is_open) {
		send(tcp_command_socket, command, 2, 0);
	}
#endif
}

void Connection::send_shutdown() {
#if CAN_SEND_COMMANDS == 1
	char command[2] = { 1, 3 };
	if (tcp_is_open) {
		send(tcp_command_socket, command, 2, 0);
	}
#endif
}

void Connection::send_rtl() {
#if CAN_SEND_COMMANDS == 1
	char command[2] = { 1, 5 };
	if (tcp_is_open) {
		send(tcp_command_socket, command, 2, 0);
	}
#endif
}

void Connection::send_auto() {
#if CAN_SEND_COMMANDS == 1
	char command[2] = { 1, 6 };
	if (tcp_is_open) {
		send(tcp_command_socket, command, 2, 0);
	}
#endif
}

double Connection::get_packets_per_second() {
	std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
	std::chrono::duration<double, std::milli> diff = timestamp - last_packet_clear;

	double millis = diff.count();
	double pps = millis == 0 ? 0 : (packet_count / millis);
	
	if (std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() > 1000) {
		packet_count = 0;
		last_packet_clear = timestamp;
	}

	return pps;
}

void Connection::setup(CurrentState* cs, std::vector<std::string>* log) {
#ifdef _WIN32
	WSADATA winblows_data;
	WSAStartup(MAKEWORD(2, 0), &winblows_data);
#endif

	should_stop = false;

	// Prepare a connection to listen for UDP from the Python client
	{
		sockaddr_in server_address;

		if ((udp_data_socket = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
			perror("Socket creation failed");
			exit(EXIT_FAILURE);
		}

		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = INADDR_ANY;
		server_address.sin_port = htons(SERVER_PORT);

		if (bind(udp_data_socket, (sockaddr*)&server_address, sizeof(server_address))) {
			fprintf(stderr, "Could not bind socket");
			closesocket(udp_data_socket);
			WSACleanup();
			exit(0);
		}

		listen(udp_data_socket, 5);
	}

#if CAN_SEND_COMMANDS == 1
	// Make a connection to the Python server
	tcp_is_open = false;
	tcp_thread = std::thread(&Connection::python_client_connect, this);
#endif

	// Start the server for the Python client
	// DO NOT REMOVE THE `THIS`
	// C++ will then create a local variable (???) throwing terminates as the variable is dereferenced??
	this->udp_thread = std::thread(&Connection::udp_server_listen, this, cs);
}

void Connection::udp_server_listen(CurrentState* cs) {
	sockaddr_in client_address;

	int client_addr_size = sizeof(client_address);

	char buffer[IDEAL_PACKET_SIZE * 2] = { 0 };
	int i = 0;
	std::string buffer_as_string;
	while (!should_stop) {
		recvfrom(udp_data_socket, buffer, IDEAL_PACKET_SIZE * 2, 0, (sockaddr*)&client_address, &client_addr_size);

		while (
			(buffer + i) < (buffer + IDEAL_PACKET_SIZE) &&
			std::memcmp((buffer + i), header, 5) != 0
		) {
			i++;
		}

		buffer_as_string = std::string((buffer + i), IDEAL_PACKET_SIZE);

		int update_status = cs->update(buffer + i, IDEAL_PACKET_SIZE);

		if (update_status != 0) {
			std::cout << "Invalid packet received; error code " << update_status << std::endl;
		}	

		packet_count++;

		usleep(8000);
	}
}

void Connection::python_client_connect() {
	sockaddr_in client_address;

	tcp_command_socket = socket(AF_INET, SOCK_STREAM, 0);

	client_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	client_address.sin_family = AF_INET;
	client_address.sin_port = htons(CLIENT_PORT);

	int status = -1;

	// Just leave the connection open
	// We can close it later when we close down in Connection::close
	while (status != 0) {
		status = connect(tcp_command_socket, (sockaddr*)&client_address, sizeof(client_address));

		if (status == -1) {
#ifdef _WIN32
			std::cout << "Connect error: " << WSAGetLastError() << std::endl;
#endif
			usleep(5000000);
		}
	}

	std::cout << "TCP Channel open" << std::endl;

	tcp_is_open = true;
}

void Connection::stop() {
	std::cout << "Closing channels" << std::endl;
	// Close outgoing channel
	send_shutdown();
	closesocket(tcp_command_socket);
#ifdef _WIN32
	WSACleanup();
#endif
	tcp_thread.join();

	// Close incoming channel
	should_stop = true;
	shutdown(udp_data_socket, 2);
	close(udp_data_socket);
	udp_thread.join();
}