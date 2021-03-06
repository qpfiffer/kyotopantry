// vim: noet ts=4 sw=4
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <msgpack.hpp>
#include <sys/types.h>
#include <zmq.hpp>

#include <string>

// Gank some stuff from Oleg:
extern "C" {
#include <oleg.h>
#include <logging.h>
}

#include "gatekeeper.h"
#include "pikeman.h"

// This is global so we can trap and gracefully shut it down.
kyotopantry::gatekeeper *mainKeeper = NULL;

void graceful_shutdown(int sig) {
	if (mainKeeper != NULL) {
		ol_log_msg(LOG_INFO, "Caught SIGINT. Shutting down gracefully.");

		zmq::context_t context(2);
		zmq::socket_t socket(context, ZMQ_REQ);
		socket.connect(SCHEDULER_URI);

		std::map<std::string, std::string> req;
		req["type"] = "shutdown";

		msgpack::sbuffer to_send;
		msgpack::pack(&to_send, req);

		zmq::message_t response(to_send.size());
		memcpy(response.data(), to_send.data(), to_send.size());
		socket.send(response);

		// We'll thread.join in the destructor:
		delete mainKeeper;
	}
	exit(0);
}

bool file_exists(const char *path) {
	int fd;
	fd = open(path, O_RDONLY);

	if (fd < 0) {
		errno = 0;
		return false;
	}
	close(fd);

	return true;
}

void main_loop(bool verbose, int num_workers) {
	int i;

	// Prepare ZMQ stuff
	zmq::context_t lcontext(1);
	zmq::socket_t lsocket(lcontext, ZMQ_REP);
	lsocket.bind(MAINLOOP_URI);

	// Spin up Pikemen
	kyotopantry::pikeman *pikemen[num_workers];
	for (i = 0; i < num_workers; i++) {
		kyotopantry::pikeman *new_recruit = new kyotopantry::pikeman();
		pikemen[i] = new_recruit;
	}

	mainKeeper->spin();

	for (i = 0; i < num_workers; i++) {
		kyotopantry::pikeman *recruit = pikemen[i];
		delete recruit;
	}
}

int main(int argc, char *argv[]) {
	srand(time(NULL));
	signal(SIGINT, graceful_shutdown);

	// Argument defaults:
	bool verbose = false;
	int num_workers = 1;


	// Look for any arguments:
	int i = 1, files_start_at = -1;
	for (i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg[0] == '-') {
			std::string cmd = arg.substr(1);

			if (cmd == "v" || cmd == "-verbose") {
				verbose = true;
			} else if (cmd == "j" || cmd == "jobs") {
				// ALL THIS FUCKING TARPIT!
				i++;
				std::string jobs_count = argv[i];
				num_workers = std::stoi(jobs_count);
				if (verbose)
					ol_log_msg(LOG_INFO, "Running with %i workers.", num_workers);
			}
		} else {
			// This isn't something that starts with a -. Assume it is a file
			// and move on.
			files_start_at = i;
			break;
		}
	}

	if (argc < 2) {
		ol_log_msg(LOG_ERR, "You need to specify some directories/files to work on.");
		return 1;
	}
	if (files_start_at == argc || files_start_at == -1) {
		ol_log_msg(LOG_ERR, "You need files to work on in addition to options.");
		return 1;
	}
	ol_log_msg(LOG_INFO, "Starting kyotopantry.");


	// Process files:
	int files_added = 0;
	mainKeeper = new kyotopantry::gatekeeper(verbose, num_workers);
	for (i = files_start_at; i < argc; i++) {
		std::string file_to_add = argv[i];

		if (!file_exists(file_to_add.c_str())) {
			ol_log_msg(LOG_WARN, "File %s doesn't exist or I can't open it or something.", argv[i]);
			continue;
		}

		if (verbose)
			ol_log_msg(LOG_INFO, "Adding %s to queue...", file_to_add.c_str());

		if (!mainKeeper->queue_file_job(file_to_add)) {
			ol_log_msg(LOG_ERR, "Could not add file to queue.");
			return 1;
		}
		files_added++;
	}

	if (files_added == 0) {
		ol_log_msg(LOG_ERR, "Could not add any files. Bummer.");
		return 1;
	}

	if (verbose)
		ol_log_msg(LOG_INFO, "Processing %i files...", files_added);

	// Actually do the processing:
	main_loop(verbose, num_workers);
	delete mainKeeper;

	return 0;
}
