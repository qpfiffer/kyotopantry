// vim: noet ts=4 sw=4
#pragma once

#include <msgpack.hpp>
#include <chrono>
#include <map>
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
	enum JobType {
		INDEX,
		DEDUPE
	};
	struct Job {
		bool being_processed;
		std::string file_path;
		int job_id;
		int job_type;

		MSGPACK_DEFINE(being_processed, file_path, job_id, job_type);
	};

	typedef std::vector<Job> JobsList;
	// Messages that the scheduler expects to receieve:
	typedef std::map<std::string, std::string> SchedulerMessage;

	class gatekeeper {
	public:
		gatekeeper(bool verbose, int num_workers);
		~gatekeeper();

		bool queue_file_job(std::string &path);
		void scheduler();
		void main_loop(bool verbose, int num_workers);
		void spin();
	private:
		// Communication with other procs
		zmq::context_t *context;
		zmq::socket_t *socket;

		// Verbosity on/off
		bool verbose;
		// How many workers we're running with
		int num_workers;
		// Handle to where we store databases
		ol_database *jobs_db;
		// Main thread
		std::thread scheduler_thread;
		// Autoincrementing job ID.
		int job_id_counter;

		void get_jobs_from_db(JobsList *jobs_list);
		bool set_job_list(JobsList &jobs_list);
		std::tuple<bool, std::string> get_next_index_job();
		std::tuple<bool, std::string> get_next_dedupe_job();
	};
}
