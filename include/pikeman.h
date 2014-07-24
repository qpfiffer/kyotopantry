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

		std::string current_file;

		// Call this when we want to notify the scheduler that we're done
		void send_shutdown();
		// Get a job from the scheduler
		bool request_job();
		// Gets an int representation of this thread's ID
		int get_thread_id();
		std::thread worker_thread;
	};
}
