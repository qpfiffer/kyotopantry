// vim: noet ts=4 sw=4
#pragma once

#define BINFO_VERSION 1
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

	// These are the structures that will be stored in the blocks_db.
	struct BlockInfo {
		const char version;
		std::string filename;
		unsigned int start;
		unsigned int length;
		std::string bmw_hash;

		BlockInfo():
			version(BINFO_VERSION), filename(""), start(0), length(4096),
			bmw_hash("") {
		}
		MSGPACK_DEFINE(filename, start, length, bmw_hash);
	};
}
