// vim: noet ts=4 sw=4
#include <iostream>
#include <msgpack.hpp>
#include <thread>
#include <zmq.hpp>
#include "gatekeeper.h"
#include "pikeman.h"

using namespace kyotopantry;

gatekeeper::gatekeeper(bool _verbose, int _num_workers) {
	this->context = new zmq::context_t(1);
	this->socket = new zmq::socket_t(*context, ZMQ_REP);
	socket->bind(SCHEDULER_URI);

	this->verbose = _verbose;
	this->num_workers = _num_workers;
	this->job_id_counter = 0;
	jobs_db = ol_open(".kyotopantry/", "jobs", OL_F_SPLAYTREE);

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

	// Create indexing Job for this file:
	Job new_pair = {
		false,
		path,
		job_id_counter,
		INDEX
	};
	jobs_list.push_back(new_pair);
	this->job_id_counter++;

	// Now create the deduplication job:
	Job new_pair_dedupe = {
		false,
		path,
		job_id_counter,
		DEDUPE
	};
	jobs_list.push_back(new_pair_dedupe);
	this->job_id_counter++;

	return set_job_list(jobs_list);
}

Job *gatekeeper::get_next_job() {
	JobsList jobs_list;
	get_jobs_from_db(&jobs_list);

	auto it = jobs_list.begin();
	bool found_job = false;
	bool found_unfinished_index_job = false;
	for (;it != jobs_list.end(); it++) {

		if (it->being_processed && it->job_type == INDEX) {
			found_unfinished_index_job = true;
		}

		// Goddammit, I accidentally made a priority queue...
		if (it->job_type == DEDUPE && found_unfinished_index_job)
			continue;

		if (it->being_processed == false) {
			found_job = true;
			break;
		}
	}

	if (found_job) {
		if (found_unfinished_index_job && it->job_type == DEDUPE) {
			ol_log_msg(LOG_INFO, "Found a dedupe job but we have unfinished index jobs.");
			Job *new_job = new Job();
			new_job->job_type = TRY_AGAIN;
			new_job->job_id = -1;
			return new_job;
		}

		it->being_processed = true;
		set_job_list(jobs_list);

		if (verbose) {
			switch (it->job_type) {
				case INDEX:
					ol_log_msg(LOG_INFO, "Found an indexing job %s", it->file_path.c_str());
					break;
				case DEDUPE:
					ol_log_msg(LOG_INFO, "Found a dedupe job %s", it->file_path.c_str());
					break;
				default:
					ol_log_msg(LOG_INFO, "Found a confused job %s", it->file_path.c_str());
					break;
			}
		}

		Job *new_job = new Job(*it);
		return new_job;
	}

	Job *new_job = new Job();
	new_job->job_type = NONE;
	new_job->job_id = -1;
	return new_job;
}

void gatekeeper::spin() {
	scheduler_thread.join();
}

void gatekeeper::send_ok_response() {
	SchedulerMessage msg;
	msg["type"] = "ok";

	msgpack::sbuffer ok;
	msgpack::pack(&ok, msg);

	zmq::message_t response(ok.size());
	memcpy(response.data(), ok.data(), ok.size());
	socket->send(response);
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
			Job *next_job = get_next_job();

			if (next_job->job_type != NONE) {
				if (verbose)
					ol_log_msg(LOG_INFO, "Scheduler sending index job.");
				SchedulerMessage new_job;
				switch (next_job->job_type) {
					case INDEX:
						new_job["type"] = "index_job";
						break;
					case DEDUPE:
						new_job["type"] = "dedupe_job";
						break;
					case TRY_AGAIN:
						new_job["type"] = "try_again";
						break;
					default: // Fall through like a boss
					case NONE:
						new_job["type"] = "no_job";
						break;
				}
				std::string next_path = next_job->file_path;
				new_job["path"] = next_path;
				new_job["id"] = std::to_string(next_job->job_id);

				msgpack::sbuffer to_send;
				msgpack::pack(&to_send, new_job);

				zmq::message_t response(to_send.size());
				memcpy(response.data(), to_send.data(), to_send.size());
				socket->send(response);
				delete next_job;
				continue;
			}

			// No jobs to send.
			SchedulerMessage new_job;
			new_job["type"] = "no_job";
			new_job["path"] = "AAAAAAAAAAAAA";

			msgpack::sbuffer what;
			msgpack::pack(&what, new_job);

			zmq::message_t new_response(what.size());
			memcpy(new_response.data(), what.data(), what.size());
			socket->send(new_response);
			delete next_job;
		} else if (resp["type"] == "job_finished") {
			if (verbose)
				ol_log_msg(LOG_INFO, "Scheduler receieved job finished.");
			send_ok_response();

		} else if (resp["type"] == "worker_end") {
			if (verbose)
				ol_log_msg(LOG_INFO, "Scheduler receieved worker shutdown.");
			send_ok_response();

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
