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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#ifdef _WIN32
#define ERROR (WSAGetLastError())
#else
#define ERROR (errno)
#endif

const char header[5] = {
	'C', 'S', 'D', 'A', 'T'
};

void Connection::open_balloon_dropper() {
#if CAN_SEND_COMMANDS == 1
	char command[3] = { 2, 1, 1 };
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, 3, 0);
	}
#endif
}

void Connection::close_balloon_dropper() {
#if CAN_SEND_COMMANDS == 1
	char command[3] = { 2, 1, 2 };
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, 3, 0);
	}
#endif
}

void Connection::toggle_arm() {
#if CAN_SEND_COMMANDS == 1
	char command[2] = { 1, 2 };
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, 2, 0);
	}
#endif
}

void Connection::send_shutdown() {
#if CAN_SEND_COMMANDS == 1
	char command[2] = { 1, 3 };
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, 2, 0);
	}
#endif
}

void Connection::send_open_waypoints(std::string file_path) {
#if CAN_SEND_COMMANDS == 1
	char size = (char)file_path.size();
	char command_size = 3 + size;
	char command[command_size] = { 2, 4, size };
	std::memcpy(command + 3, file_path.data(), size);
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, command_size, 0);
	}
#endif
}

void Connection::send_rtl() {
#if CAN_SEND_COMMANDS == 1
	char command[2] = { 1, 5 };
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, 2, 0);
	}
#endif
}

void Connection::send_auto() {
#if CAN_SEND_COMMANDS == 1
	char command[2] = { 1, 6 };
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, 2, 0);
	}
#endif
}

void Connection::send_command(char* command, size_t length) {
#if CAN_SEND_COMMANDS == 1
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, length, 0);
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

int Connection::setup(CurrentState* cs, std::vector<std::string>* log) {
#ifdef _WIN32
	WSADATA winblows_data;
	WSAStartup(MAKEWORD(2, 0), &winblows_data);
#endif

	should_stop = false;

	// Prepare a connection to listen for UDP from the Python client
	{
		sockaddr_in server_address;

		if ((udp_data_socket = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
			std::cerr << "UDP socket creation failed: " << ERROR << std::endl;
			return ERROR;
		}

		struct timeval timeout_val;
		timeout_val.tv_sec = 0;
		timeout_val.tv_usec = 10000;

#ifdef _WIN32
		if (setsockopt(udp_data_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_val, sizeof(timeout_val)) < 0) {
#else
		if (setsockopt(udp_data_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_val, sizeof(timeout_val)) < 0) {
#endif
			std::cerr << "UDP socket timeout set failure: " << ERROR << std::endl;
#ifdef _WIN32
			closesocket(udp_data_socket);
			WSACleanup();
#else
			close(udp_data_socket);
#endif
			return ERROR;
		}

		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		server_address.sin_port = htons(DATA_PORT);

		if (bind(udp_data_socket, (sockaddr*)&server_address, sizeof(server_address))) {
			std::cerr << "Could not bind UDP socket: " << ERROR << std::endl;
#ifdef _WIN32
			closesocket(udp_data_socket);
			WSACleanup();
#else
			close(udp_data_socket);
#endif
			return EXIT_FAILURE;
		}

		listen(udp_data_socket, 5);
	}

#if CAN_SEND_COMMANDS == 1
	{
		sockaddr_in server_address;
#ifdef _WIN32
		char opt = 1;
#else
		int opt = 1;
#endif

		if ((tcp_command_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
			std::cerr << "TCP socket creation failed: " << ERROR << std::endl;
			return ERROR;
		}

		if (setsockopt(tcp_command_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == SOCKET_ERROR) {
			std::cerr << "Failed to setup TCP socket" << ERROR << std::endl;
			return ERROR;
		}

		struct timeval timeout_val;
		timeout_val.tv_sec = 0;
		timeout_val.tv_usec = 10000;

#ifdef _WIN32
		if (setsockopt(tcp_command_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_val, sizeof(timeout_val)) < 0) {
#else
		if (setsockopt(tcp_command_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_val, sizeof(timeout_val)) < 0) {
#endif
			std::cerr << "Could not set TCP socket timeout: " << ERROR << std::endl;
			return ERROR;
		}

		server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(COMMAND_PORT);

		if (bind(tcp_command_socket, (sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
			std::cerr << "Failed to bind TCP server: " << ERROR << std::endl;
			return ERROR;
		}

		if (listen(tcp_command_socket, 3) == SOCKET_ERROR) {
			std::cerr << "Failed to set TCP server to listen: " << ERROR << std::endl;
			return ERROR;
		}
	}

	// Start listening for client connections
	this->tcp_thread = std::thread(&Connection::tcp_server_listen, this);
#endif

	// Start the server for the Python client
	// DO NOT REMOVE THE `THIS`
	// C++ will then create a local variable (???) throwing terminates as the variable is dereferenced??
	this->udp_thread = std::thread(&Connection::udp_server_listen, this, cs);

	return EXIT_SUCCESS;
}

void Connection::udp_server_listen(CurrentState* cs) {
	sockaddr_in client_address;

#ifdef _WIN32
	int client_addr_size = sizeof(client_address);
#else
	socklen_t client_addr_size = sizeof(client_address);
#endif

	char buffer[IDEAL_PACKET_SIZE] = { 0 };
	int i = 0;
	std::string buffer_as_string;
	size_t received = 0;
	while (!should_stop) {
		received = recvfrom(udp_data_socket, buffer, IDEAL_PACKET_SIZE * 2, 0, (sockaddr*)&client_address, &client_addr_size);

		if (received != IDEAL_PACKET_SIZE) {
			continue;
		}

		while (
			(buffer + i) < (buffer + IDEAL_PACKET_SIZE) &&
			std::memcmp((buffer + i), header, 5) != 0
		) {
			i++;
		}

		buffer_as_string = std::string((buffer + i), IDEAL_PACKET_SIZE);

		int update_status = cs->update(buffer + i, IDEAL_PACKET_SIZE);

		if (update_status != 0) {
			std::cerr << "Invalid packet received; error code " << update_status << std::endl;
		}	

		packet_count++;
	}
}

void Connection::tcp_server_listen() {
	SOCKET new_socket;
	sockaddr_in client_address;
#ifdef _WIN32
	int c;
#else
	socklen_t c;
#endif

	c = sizeof(struct sockaddr_in);

	while (!should_stop) {
		new_socket = accept(tcp_command_socket, (struct sockaddr*)&client_address, &c);
		if (new_socket != INVALID_SOCKET) {
			tcp_command_sockets.push_back(new_socket);
		}
	}

#ifdef _WIN32
	closesocket(tcp_command_socket);
	WSACleanup();
#else
	close(tcp_command_socket);
#endif
}

void Connection::stop() {
	should_stop = true;

	// Close outgoing channel
	send_shutdown();
#ifdef _WIN32
	for (auto& c_sock : tcp_command_sockets) {
		closesocket(c_sock);
	}
	closesocket(tcp_command_socket);
	WSACleanup();
#else
	for (auto& c_sock : tcp_command_sockets) {
		close(c_sock);
	}
	close(tcp_command_socket);
#endif

	tcp_thread.join();

	// Close incoming channel
	shutdown(udp_data_socket, 2);
	close(udp_data_socket);
	udp_thread.join();
}
