#include "Connection.h"
#include "../data/CurrentState.h"
#include <thread>
#include <winsock.h>
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <unistd.h>

#define CLIENT_PORT 1337
#define SERVER_PORT 54248

void Connection::open_charlie_bottle() {
	char command[4] = { 3, 1, 5, 1 };
	send(tcp_command_socket, command, 4, 0);
}

void Connection::close_charlie_bottle() {
	char command[4] = { 3, 1, 5, 2 };
	send(tcp_command_socket, command, 4, 0);
}

void Connection::open_golf_bottle() {
	char command[4] = { 3, 1, 6, 1 };
	send(tcp_command_socket, command, 4, 0);
}

void Connection::close_golf_bottle() {
	char command[4] = { 3, 1, 6, 2 };
	send(tcp_command_socket, command, 4, 0);
}

void Connection::toggle_arm() {
	char command[2] = { 1, 2 };
	send(tcp_command_socket, command, 2, 0);
}

void Connection::send_shutdown() {
	char command[2] = { 1, 3 };
	send(tcp_command_socket, command, 2, 0);
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
	// Start the server for the Python client
	udp_thread = std::thread(&Connection::server_thread, this, cs, log);

	// Make a connection to the Python server
	{
		SOCKADDR_IN server_address;
		WSADATA WSAData;
		WSAStartup(MAKEWORD(2,0), &WSAData);

		tcp_command_socket = socket(AF_INET, SOCK_STREAM, 0);

		server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(CLIENT_PORT);

		// Just leave the connection hanging
		// We can close it later when we close down in Connection::close
		connect(tcp_command_socket, (SOCKADDR*)&server_address, sizeof(server_address));
	}
}

void Connection::server_thread(CurrentState* cs, std::vector<std::string>* log) {
	SOCKADDR_IN server_address, client_address;
	SOCKET client;

	char buffer[IDEAL_PACKET_SIZE] = { 0 };

	if ((udp_data_socket = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(SERVER_PORT);

	bind(udp_data_socket, (SOCKADDR*)&server_address, sizeof(server_address));
	listen(udp_data_socket, 0);

	int client_addr_size = sizeof(client_address);
	while ((client = accept(udp_data_socket, (SOCKADDR*)&client_address, &client_addr_size)) != INVALID_SOCKET) {
		recv(client, buffer, sizeof(buffer), 0);

		int update_status = cs->update(buffer);

		std::cout << "Received packet" << std::endl;

		if (update_status != 0) {
			std::cout << "Invalid packet received; error code " << update_status << std::endl;
		}

		packet_count++;

		closesocket(client);
	}
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