// vim: noet ts=4 sw=4
#include <msgpack.hpp>
#include <zmq.hpp>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

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

	const int moons_of_jupiter_length = sizeof(moons_of_jupiter) / sizeof(char *);
	this->thread_name = std::string(moons_of_jupiter[rand() % moons_of_jupiter_length]);

	worker_thread = std::thread(&pikeman::do_work, this);
}

pikeman::~pikeman() {
	worker_thread.join();

	delete socket;
	delete context;
}

bool pikeman::request_job() {
	SchedulerMessage job_request;
	job_request["type"] = "job_request";

	msgpack::sbuffer request;
	msgpack::pack(&request, job_request);

	zmq::message_t zrequest((void *)request.data(), request.size(), NULL);
	ol_log_msg(LOG_INFO, "%s: Sending request for job.", thread_name.c_str());
	socket->send(zrequest);

	zmq::message_t new_job_resp;
	assert(socket->recv(&new_job_resp) == true);

	if (new_job_resp.size() == 0) {
		ol_log_msg(LOG_WARN, "%s: Received no job. Shutting down.", thread_name.c_str());

		return false;
	}

	this->current_file_name = std::string(static_cast<char *>(new_job_resp.data()), new_job_resp.size());
	ol_log_msg(LOG_INFO, "%s: Received job %s.", thread_name.c_str(), this->current_file_name.c_str());

	return true;
}

bool pikeman::open_job() {
	struct stat sb = {0};
	int fd = open(this->current_file_name.c_str(), O_RDONLY);

	if (fd <= 0) {
		ol_log_msg(LOG_WARN, "%s: Could not open file %s.",
				thread_name.c_str(), this->current_file_name.c_str());
		errno = 0;
		return false;
	}

	if (fstat(fd, &sb) == -1) {
		ol_log_msg(LOG_WARN, "%s: Could not open file %s.",
				thread_name.c_str(), this->current_file_name.c_str());
		errno = 0;
		close(fd);
		return false;
	}

	current_file_size = sb.st_size;

	ol_log_msg(LOG_INFO, "Working on file if size %i.", current_file_size);

	current_file = mmap(NULL, current_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (current_file == MAP_FAILED) {
		ol_log_msg(LOG_WARN, "Could not mmap file.");
		close(fd);
		return false;
	}

	close(fd);
	return true;
}

void pikeman::send_shutdown() {
	SchedulerMessage job_request;
	job_request["type"] = "worker_end";

	msgpack::sbuffer request;
	msgpack::pack(&request, job_request);

	zmq::message_t zrequest((void *)request.data(), request.size(), NULL);

	socket->send(zrequest);

	zmq::message_t server_response;
	assert(socket->recv(&server_response) == true);
}

void pikeman::do_work() {
	ol_log_msg(LOG_INFO, "%s: Doing some work.", thread_name.c_str());

	socket->connect(SCHEDULER_URI);
	while (request_job()) {
		// Do some goddamn WORK bro
		// 1. mmap() file into memory
		if (!open_job())
			continue;
		// 2. Scan through 4k chunks at a time
		// 3. Ask the database if there exists a chunk with that hash
		// 4. ???????
		// 5. File is deduplicated!
	}

	send_shutdown();
}
