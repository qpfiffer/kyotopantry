// vim: noet ts=4 sw=4
#pragma once
#include <thread>

// Pikemen are responsible for processing individual files.
namespace kyotopantry {
	class pikeman {
	public:
		pikeman();
		~pikeman();

		void do_work();
	private:
		zmq::context_t *context;
		zmq::socket_t *socket;

		void request_job();
		int get_thread_id();
		std::thread worker_thread;
	};
}
