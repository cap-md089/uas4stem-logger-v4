#include "Connection.h"
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>

void Connection::toggle_arm() {

}

void Connection::open_container(UAVBottles target) {

}

void Connection::close_container(UAVBottles target) {

}

void Connection::start_thread(CurrentState* cs) {
	if(!udp_thread) {
		udp_thread = new std::thread(&Connection::thread_runner, this, cs);
	}
}

#define PORT 54248

void Connection::thread_runner(CurrentState* cs) {
	int new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[91] = { 0 };

	if ((udp_data_socket = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	bind(udp_data_socket, (struct sockaddr*)&address, sizeof(address));


}
