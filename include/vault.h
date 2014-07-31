// vim: noet ts=4 sw=4
#pragma once

#define B_FAILED_TO_OPEN 601
#define VAULT_URI "ipc://.vault.sock"

#include <thread>
#include <zmq.hpp>

extern "C" {
#include <oleg.h>
#include <logging.h>
}

namespace kyotopantry {
	// The vault is responsible for storing duplicated blocks and hashes of
	// blocks that have been seen before.
	class vault {
	public:
		vault();
		~vault();

		// Main thread:
		void do_work();
		void spin();
	private:
		ol_database *block_db;

		zmq::context_t *context;
		zmq::socket_t *socket;

		std::thread vault_thread;
	};
}
