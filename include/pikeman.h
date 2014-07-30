// vim: noet ts=4 sw=4
#pragma once
#include <thread>
#include "jobtypes.h"

// Pikemen are responsible for processing individual files.
namespace kyotopantry {

	class pikeman {
	public:
		pikeman();
		~pikeman();

		void do_work();
	private:
		std::string thread_name;

		zmq::context_t *context;
		zmq::socket_t *socket;

		filejob *current_job;

		// Call this when we want to notify the scheduler that we're done
		void send_shutdown();
		// Get a job from the scheduler
		bool request_job();
		void send_job_success(const int job_id);
		void send_job_failure(const int job_id);
		std::thread worker_thread;
	};
}
