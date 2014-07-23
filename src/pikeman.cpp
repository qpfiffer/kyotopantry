#include <msgpack.hpp>
#include <zmq.hpp>
#include <sstream>

#include "pikeman.h"
#include "gatekeeper.h"

using namespace kyotopantry;

pikeman::pikeman() {
	worker_thread = std::thread(&pikeman::do_work, this);
}

pikeman::~pikeman() {
	worker_thread.join();
}

int pikeman::get_thread_id() {
	std::stringstream s;
	std::thread::id pid = worker_thread.get_id();
	s << pid;
	return stoi(s.str().substr(6));
}

void pikeman::do_work() {
	ol_log_msg(LOG_INFO, "Thread %i Doing some work.", this->get_thread_id());

	zmq::context_t context(2);
	zmq::socket_t socket(context, ZMQ_REQ);
	socket.connect(SCHEDULER_URI);

	std::map<std::string, std::string> job_request;
	job_request["type"] = "job_request";

	msgpack::sbuffer *request = new msgpack::sbuffer;
	msgpack::pack(request, job_request);

	zmq::message_t zrequest(request->size());
	memcpy((void *)zrequest.data(), request->data(), request->size());
	ol_log_msg(LOG_INFO, "Thread %i Sending request for job.", this->get_thread_id());
	socket.send(zrequest);

	zmq::message_t new_job_resp;
	assert(socket.recv(&new_job_resp) == true);

	ol_log_msg(LOG_INFO, "Thread %i Received job %s.", this->get_thread_id(), new_job_resp.data());
}
