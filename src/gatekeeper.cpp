// vim: noet ts=4 sw=4
#include <iostream>
#include <msgpack.hpp>
#include <thread>
#include <zmq.hpp>
#include "gatekeeper.h"
#include "pikeman.h"

using namespace kyotopantry;

gatekeeper::gatekeeper(bool verbose, int num_workers) {
	this->context = new zmq::context_t(1);
	this->socket = new zmq::socket_t(*context, ZMQ_REP);
	socket->bind(SCHEDULER_URI);

	this->verbose = verbose;
	this->num_workers = num_workers;
	jobs_db = ol_open(".kyotopantry/", "jobs", OL_F_SPLAYTREE | OL_F_LZ4);

	if (jobs_db == NULL) {
		throw GK_FAILED_TO_OPEN;
	}

	JobsList jobs_list;

	msgpack::sbuffer to_save;
	msgpack::pack(&to_save, jobs_list);

	// TODO: See if we have unprocessed jobs.
	int ret = ol_jar(jobs_db, JOBS_LIST, strlen(JOBS_LIST), (unsigned char *)to_save.data(), to_save.size());
	assert(ret == 0);

	scheduler_thread = std::thread(&gatekeeper::scheduler, this);
}

gatekeeper::~gatekeeper() {
	try {
		scheduler_thread.join();
	} catch (std::system_error) {
		// Somebody already joined this thread. Asshole.
	}

	ol_close(jobs_db);

	delete socket;
	delete context;
}

void gatekeeper::get_jobs_from_db(JobsList *jobs_list) {
	size_t datasize;
	unsigned char *job_results = NULL;
	// Pull jobs from db
	assert(ol_unjar_ds(jobs_db, JOBS_LIST, strlen(JOBS_LIST), &job_results, &datasize) == 0);

	msgpack::object obj;
	msgpack::unpacked unpacked;

	msgpack::unpack(&unpacked, (char *)job_results, datasize);
	obj = unpacked.get();

	obj.convert(jobs_list);

	free(job_results);
}

bool gatekeeper::set_job_list(JobsList &jobs_list) {
	// Save jobs back to db
	msgpack::sbuffer to_save;
	msgpack::pack(&to_save, jobs_list);

	int ret = ol_jar(jobs_db, JOBS_LIST, strlen(JOBS_LIST), (unsigned char *)to_save.data(), to_save.size());

	if (ret == 0)
		return true;
	return false;
}

bool gatekeeper::queue_file_job(std::string &path) {
	JobsList jobs_list;
	get_jobs_from_db(&jobs_list);

	// Actually add it to the thing in the DB
	if (verbose)
		ol_log_msg(LOG_INFO, "Saving job %s", path.c_str());
	Job new_pair = std::make_pair(false, path);
	jobs_list.push_back(new_pair);

	return set_job_list(jobs_list);
}

std::string gatekeeper::get_next_job() {
	JobsList jobs_list;
	get_jobs_from_db(&jobs_list);

	auto it = jobs_list.begin();
	bool found_job = false;
	Job job;
	for (;it != jobs_list.end(); it++) {
		job = *it;

		if (job.first == false) {
			found_job = true;
			if (verbose)
				ol_log_msg(LOG_INFO, "Found job %s", job.second.c_str());
			break;
		}
	}

	if (found_job) {
		it->first = true;
		set_job_list(jobs_list);

		return job.second;
	}

	return "";
}

void gatekeeper::spin() {
	scheduler_thread.join();
}

void gatekeeper::scheduler() {
	//zmq::socket_t main_loop_socket(*context, ZMQ_REQ);
	//main_loop_socket.connect(MAINLOOP_URI);

	// Process job requests from pikemen:
	while (true) {
		SchedulerMessage resp;
		zmq::message_t request;
		if (verbose)
			ol_log_msg(LOG_INFO, "Scheduler waiting.");
		assert(socket->recv(&request) == true);

		msgpack::object obj;
		msgpack::unpacked unpacked;

		msgpack::unpack(&unpacked, (char *)request.data(), request.size());
		obj = unpacked.get();
		obj.convert(&resp);


		if (resp["type"] == "job_request") {
			if (verbose)
				ol_log_msg(LOG_INFO, "Scheduler receieved job request.");
			std::string next_file = get_next_job();

			zmq::message_t response((void *)next_file.data(), next_file.size(), NULL);
			socket->send(response);

		} else if (resp["type"] == "job_finished") {
			if (verbose)
				ol_log_msg(LOG_INFO, "Scheduler receieved job finished.");

		} else if (resp["type"] == "worker_end") {
			if (verbose)
				ol_log_msg(LOG_INFO, "Scheduler receieved worker shutdown.");

			std::string ok = "ok";
			zmq::message_t response((void *)ok.data(), ok.size(), NULL);
			socket->send(response);

			num_workers--;
			if (num_workers <= 0) {
				ol_log_msg(LOG_ERR, "No more workers. Shutting down.");
				break;
			}
		} else if (resp["type"] == "shutdown") {
			if (verbose)
				ol_log_msg(LOG_INFO, "Scheduler received shutdown request.");

			break;
		}
	}
}
