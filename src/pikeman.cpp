// vim: noet ts=4 sw=4
#include <msgpack.hpp>
#include <zmq.hpp>
#include <sstream>

#include "pikeman.h"
#include "gatekeeper.h"

using namespace kyotopantry;

// Used for naming threads something interesting.
static const char *moons_of_jupiter[] = {
	"Adrastea",
	"Aitne",
	"Amalthea",
	"Ananke",
	"Aoede",
	"Arche",
	"Autonoe",
	"Callirrhoe",
	"Callisto",
	"Carme",
	"Carpo",
	"Chaldene",
	"Cyllene",
	"Elara",
	"Erinome",
	"Euanthe",
	"Eukelade",
	"Euporie",
	"Europa",
	"Eurydome",
	"Ganymede",
	"Harpalyke",
	"Hegemone",
	"Helike",
	"Hermippe",
	"Herse",
	"Himalia",
	"Io",
	"Iocaste",
	"Isonoe",
	"Kale",
	"Kallichore",
	"Kalyke",
	"Kore",
	"Leda",
	"Lysithea",
	"Megaclite",
	"Metis",
	"Mneme",
	"Orthosie",
	"Pasiphaë",
	"Pasithee",
	"Praxidike",
	"Sinope",
	"Spider",
	"Sponde",
	"Taygete",
	"Thebe",
	"Thelxinoe",
	"Themisto",
	"Thyone"
};

pikeman::pikeman() {
	this->context = new zmq::context_t(1);
	this->socket = new zmq::socket_t(*context, ZMQ_REQ);

	this->current_job = NULL;

	const int moons_of_jupiter_length = sizeof(moons_of_jupiter) / sizeof(char *);
	this->thread_name = std::string(moons_of_jupiter[rand() % moons_of_jupiter_length]);

	worker_thread = std::thread(&pikeman::do_work, this);
}

pikeman::~pikeman() {
	try {
		worker_thread.join();
	} catch (std::system_error) {
		// Somebody already joined this thread. Asshole.
	}

	delete current_job;
	delete socket;
	delete context;
}

bool pikeman::request_job() {
	SchedulerMessage job_request;
	job_request["type"] = "job_request";

	msgpack::sbuffer request;
	msgpack::pack(&request, job_request);

	zmq::message_t zrequest(request.size());
	memcpy(zrequest.data(), request.data(), request.size());
	ol_log_msg(LOG_INFO, "%s: Sending request for job.", thread_name.c_str());
	socket->send(zrequest);

	zmq::message_t new_job_resp;
	assert(socket->recv(&new_job_resp) == true);

	if (new_job_resp.size() == 0) {
		ol_log_msg(LOG_WARN, "%s: Received empty job. Shutting down.", thread_name.c_str());
		return false;
	}

	msgpack::object obj;
	msgpack::unpacked unpacked;

	msgpack::unpack(&unpacked, (char *)new_job_resp.data(), new_job_resp.size());
	obj = unpacked.get();

	//msgpack_object_print(stdout, obj);
	//ol_log_msg(LOG_WARN, "Receieved that.");

	SchedulerMessage msg;
	obj.convert(&msg);

	if (msg["type"] == "no_job") {
		ol_log_msg(LOG_WARN, "%s: Received no job. Shutting down.", thread_name.c_str());
		return false;
	}

	delete this->current_job; // Remove the old one.
	if (msg["type"] == "index_job") {
		this->current_job = new indexjob(msg["path"], std::stoi(msg["id"]));
	} else if (msg["type"] == "dedupe_job") {
		this->current_job = new dedupejob(msg["path"], std::stoi(msg["id"]));
	} else if (msg["type"] == "try_again") {
		// This job doesn't really do anything than sit here.
		this->current_job = new sleepjob("", -1);
	}

	ol_log_msg(LOG_INFO, "%s: Received job %s.", thread_name.c_str(), current_job->get_current_file_name().c_str());

	return true;
}

void pikeman::send_shutdown() {
	SchedulerMessage job_request;
	job_request["type"] = "worker_end";

	msgpack::sbuffer request;
	msgpack::pack(&request, job_request);

	zmq::message_t zrequest(request.size());
	memcpy(zrequest.data(), request.data(), request.size());

	socket->send(zrequest);

	zmq::message_t server_response;
	assert(socket->recv(&server_response) == true);
}

void pikeman::do_work() {
	ol_log_msg(LOG_INFO, "%s: Doing some work.", thread_name.c_str());

	socket->connect(SCHEDULER_URI);
	while (request_job()) {
		// Do some goddamn WORK bro
		if (this->current_job->do_job()) {
		} else {
			ol_log_msg(LOG_WARN, "%s: Could not complete job!.", thread_name.c_str());
			// TODO: Tell the gatekeeper we were not able to complete the job
			// we were assigned.
		}
	}

	send_shutdown();
}
