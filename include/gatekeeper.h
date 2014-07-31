// vim: noet ts=4 sw=4
#pragma once

#define GK_FAILED_TO_OPEN 600

#define MAINLOOP_URI "ipc://.mainloop.sock"
#define SCHEDULER_URI "ipc://.scheduler.sock"

#define JOBS_LIST "all_jobs"

#include <msgpack.hpp>
#include <chrono>
#include <map>
#include <string>
#include <thread>
#include "vault.h"

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
		DEDUPE,
		TRY_AGAIN,
		NONE
	};
	struct Job {
		bool being_processed;
		std::string file_path;
		int job_id;
		unsigned char job_type;
		// ^^^ Actually a JobType enum but MSGPACK doesn't know how to
		// serialize those so we pretend it's a char.

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
		// How we talk to the vault:
		zmq::socket_t *vsocket;


		// This is the main vault instance.
		vault *theVault;

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

		// Deserializes the job list from the database
		void get_jobs_from_db(JobsList *jobs_list);
		// Sets the job list in the persistent DB
		bool set_job_list(JobsList &jobs_list);
		// Gets the next available job:
		Job *get_next_job();
		// Generic helper method to respond to workers:
		void send_ok_response();
		bool mark_job_as_done(const int job_id);
		void send_shutdown();
	};
}
