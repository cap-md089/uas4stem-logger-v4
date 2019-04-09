#include "Connection.h"
#include "../data/CurrentState.h"
#include <thread>
#include <winsock.h>
#include <string>
#include <vector>
#include <chrono>
#include <unistd.h>

#include <iostream>
#include <stdio.h>

#define CLIENT_PORT 1337
#define SERVER_PORT 54248

#define CAN_SEND_COMMANDS 1

void Connection::open_charlie_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 5, 1 };
	send(tcp_command_socket, command, 4, 0);
#endif
}

void Connection::close_charlie_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 5, 2 };
	send(tcp_command_socket, command, 4, 0);
#endif
}

void Connection::open_golf_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 6, 1 };
	send(tcp_command_socket, command, 4, 0);
#endif
}

void Connection::close_golf_bottle() {
#if CAN_SEND_COMMANDS == 1
	char command[4] = { 3, 1, 6, 2 };
	send(tcp_command_socket, command, 4, 0);
#endif
}

void Connection::toggle_arm() {
#if CAN_SEND_COMMANDS == 1
	char command[2] = { 1, 2 };
	send(tcp_command_socket, command, 2, 0);
#endif
}

void Connection::send_shutdown() {
#if CAN_SEND_COMMANDS == 1
	char command[2] = { 1, 3 };
	send(tcp_command_socket, command, 2, 0);
#endif
}

double Connection::get_packets_per_second() {
	std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
	std::chrono::duration<double, std::milli> diff = timestamp - last_packet_clear;

	double millis = diff.count();
	double pps = millis == 0 ? 0 : (packet_count / millis);
	
	if (std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() > 10) {
		packet_count = 0;
		last_packet_clear = timestamp;
	}

	return pps;
}

void Connection::setup(CurrentState* cs, std::vector<std::string>* log) {
	WSADATA winblows_data;
	WSAStartup(MAKEWORD(2, 0), &winblows_data);

	// Prepare a connection to listen for UDP from the Python client
	{
		SOCKADDR_IN server_address;

		if ((udp_data_socket = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
			perror("Socket creation failed");
			exit(EXIT_FAILURE);
		}

		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = INADDR_ANY;
		server_address.sin_port = htons(SERVER_PORT);

		if (bind(udp_data_socket, (SOCKADDR*)&server_address, sizeof(server_address))) {
			fprintf(stderr, "Could not bind socket");
			closesocket(udp_data_socket);
			WSACleanup();
			exit(0);
		}

		listen(udp_data_socket, 5);
	}

	// Start the server for the Python client
	udp_thread = std::thread(&Connection::server_thread, this, cs, log);

#if CAN_SEND_COMMANDS == 1
	// Make a connection to the Python server
	{
		SOCKADDR_IN client_address;

		tcp_command_socket = socket(AF_INET, SOCK_STREAM, 0);

		client_address.sin_addr.s_addr = inet_addr("127.0.0.1");
		client_address.sin_family = AF_INET;
		client_address.sin_port = htons(CLIENT_PORT);

		// Just leave the connection hanging
		// We can close it later when we close down in Connection::close
		connect(tcp_command_socket, (SOCKADDR*)&client_address, sizeof(client_address));
	}
#endif
}

void Connection::server_thread(CurrentState* cs, std::vector<std::string>* log) {
	SOCKADDR_IN client_address;

	int client_addr_size = sizeof(client_address);

	char buffer[IDEAL_PACKET_SIZE] = { 0 };
	std::string buffer_as_string, buffer2, buffer3;

	std::cout << "Got here to starting thread" << std::endl;

	while (1) {
		recvfrom(udp_data_socket, buffer, sizeof(buffer), 0, (SOCKADDR*)&client_address, &client_addr_size);

		buffer_as_string = std::string(buffer, IDEAL_PACKET_SIZE);
		buffer2 = std::string(buffer + 0x59, 5);
		buffer3 = std::string(buffer + 0x65, 5);

		int update_status = cs->update(buffer_as_string);

		if (update_status != 0) {
			int i;
			for (i = 0; i < IDEAL_PACKET_SIZE; i++) {
				if (*(buffer + i)) {
					printf("(%#2X: %#2X)", i, *(buffer + i));
				}
			}
			printf("\n\n\n");
			//std::cout << "Invalid packet received; error code " << update_status << std::endl;
		}	

		packet_count++;

		usleep(8333);
	}

	std::cout << WSAGetLastError() << std::endl;
	std::cout << "Invalid socket, ended" << std::endl;
}

void Connection::stop() {
	// Close outgoing channel
	send_shutdown();
	closesocket(tcp_command_socket);
	WSACleanup();

	// Close incoming channel
	shutdown(udp_data_socket, 2);
	close(udp_data_socket);
	udp_thread.join();
}