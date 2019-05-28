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
#endif

const char header[5] = {
	'C', 'S', 'D', 'A', 'T'
};

void Connection::open_charlie_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 5, 1 };
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, 4, 0);
	}
#endif
}

void Connection::close_charlie_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 5, 2 };
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, 4, 0);
	}
#endif
}

void Connection::open_golf_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 6, 1 };
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, 4, 0);
	}
#endif
}

void Connection::close_golf_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 6, 2 };
	for (auto& tcp_command_socket : tcp_command_sockets) {
		send(tcp_command_socket, command, 4, 0);
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
#ifdef _WIN32
			closesocket(udp_data_socket);
			WSACleanup();
#else
			close(udp_data_socket);
#endif
			exit(0);
		}

		listen(udp_data_socket, 5);
	}

#if CAN_SEND_COMMANDS == 1
	// Start listening for client connections
	tcp_thread = std::thread(&Connection::tcp_server_listen, this);
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
	}
}

void Connection::tcp_server_listen() {
	WSADATA wsa;
	SOCKET new_socket;
	sockaddr_in client_address, server_address;
	int c;

	if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
		std::cout << "Failed to initialize server (0): " << WSAGetLastError() << std::endl;
		return;
	}

	if ((tcp_server = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		std::cout << "Failed to initialize server (1): " << WSAGetLastError() << std::endl;
		return;
	}

	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(CLIENT_PORT);

	if (bind(tcp_server, (sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
		std::cout << "Failed to bind server: " << WSAGetLastError() << std::endl;
		return;
	}

	listen(tcp_server, 3);

	c = sizeof(struct sockaddr_in);

	while (!should_stop) {
		new_socket = accept(tcp_server, (struct sockaddr*)&client_address, &c);
		if (new_socket == INVALID_SOCKET) {
			std::cout << "Invalid connection incoming: " << WSAGetLastError() << std::endl;
		} else {
			tcp_command_sockets.push_back(new_socket);
		}
	}

	closesocket(tcp_server);
	WSACleanup();
}

void Connection::stop() {
	std::cout << "Closing channels" << std::endl;
	// Close outgoing channel
	send_shutdown();
	for (auto& tcp_command_socket : tcp_command_sockets) {
		closesocket(tcp_command_socket);
	}
#ifdef _WIN32
	closesocket(tcp_server);
	WSACleanup();
#else
	close(tcp_server);
#endif
	tcp_thread.join();

	// Close incoming channel
	should_stop = true;
	shutdown(udp_data_socket, 2);
	close(udp_data_socket);
	udp_thread.join();
}
