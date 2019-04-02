#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <thread>

enum UAVBottles {
	LEFT_BOTTLE,
	RIGHT_BOTTLE
};

class Connection {
	public:
		void is_connected();
		void toggle_arm();
		void open_container(UAVBottles target);
		void close_container(UAVBottles target);

		int get_packets_per_second();

		void start_thread(CurrentState* cs);
	private:
		int udp_data_socket;
		int tcp_command_socket;

		void thread_runner(CurrentState* cs);

		std::thread udp_thread;
};

#endif
