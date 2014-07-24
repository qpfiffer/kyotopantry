// vim: noet ts=4 sw=4
#include <msgpack.hpp>
#include <zmq.hpp>
#include <sstream>

#include "pikeman.h"
#include "gatekeeper.h"

using namespace kyotopantry;

pikeman::pikeman() {
	this->context = new zmq::context_t(1);
	this->socket = new zmq::socket_t(*context, ZMQ_REQ);

	worker_thread = std::thread(&pikeman::do_work, this);
}

pikeman::~pikeman() {
	worker_thread.join();

	delete socket;
	delete context;
}

int pikeman::get_thread_id() {
	std::stringstream s;
	std::thread::id pid = worker_thread.get_id();
	s << pid;
	return stoi(s.str().substr(6));
}

bool pikeman::request_job() {
	SchedulerMessage job_request;
	job_request["type"] = "job_request";

	msgpack::sbuffer request;
	msgpack::pack(&request, job_request);

	zmq::message_t zrequest(request.size());
	memcpy((void *)zrequest.data(), request.data(), request.size());
	ol_log_msg(LOG_INFO, "Thread %i Sending request for job.", this->get_thread_id());
	socket->send(zrequest);

	zmq::message_t new_job_resp;
	assert(socket->recv(&new_job_resp) == true);

	if (new_job_resp.size() == 0) {
		ol_log_msg(LOG_WARN, "Thread %i Received no job. Shutting down.", this->get_thread_id());

		return false;
	}

	ol_log_msg(LOG_INFO, "Thread %i Received job %s.", this->get_thread_id(), new_job_resp.data());

	return true;
}

void pikeman::send_shutdown() {
	SchedulerMessage job_request;
	job_request["type"] = "worker_end";

	msgpack::sbuffer request;
	msgpack::pack(&request, job_request);

	zmq::message_t zrequest(request.size());
	memcpy((void *)zrequest.data(), request.data(), request.size());

	socket->send(zrequest);

	zmq::message_t server_response;
	assert(socket->recv(&server_response) == true);
}

void pikeman::do_work() {
	ol_log_msg(LOG_INFO, "Thread %i Doing some work.", this->get_thread_id());

	socket->connect(SCHEDULER_URI);
	while (request_job()) {
		// Do some goddamn WORK bro
		// 1. mmap() file into memory
		// 2. Scan through 4k chunks at a time
		// 3. Ask the database if there exists a chunk with that hash
		// 4. ???????
		// 5. File is deduplicated!
	}

	send_shutdown();
}
