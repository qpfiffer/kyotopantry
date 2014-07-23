#pragma once

#include <chrono>
#include <string>
#include <thread>

#define GK_FAILED_TO_OPEN 600

#define MAINLOOP_URI "ipc://.mainloop.sock"
#define SCHEDULER_URI "ipc://.scheduler.sock"

#define JOBS_LIST "all_jobs"

extern "C" {
#include <oleg.h>
#include <logging.h>
}

namespace kyotopantry {
	// The job list is a list of tuples {<being processed>, <file path>}. Each
	// tuple is initialized to false, and set to true when it is being worked
	// on.
	typedef std::vector<std::pair<bool, std::string>> JobsList;

	class gatekeeper {
	public:
		gatekeeper(bool verbose);
		~gatekeeper();

		bool queue_file_job(std::string &path);
		void scheduler();
		void main_loop(bool verbose, int num_workers);
	private:
		bool verbose;
		ol_database *jobs_db;
		std::thread scheduler_thread;

		void get_jobs_from_db(JobsList **jobs_list);
		bool set_job_list(JobsList &jobs_list);
		std::string get_next_job();
	};
}
