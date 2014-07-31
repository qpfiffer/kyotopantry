// vim: noet ts=4 sw=4
#pragma once

extern "C" {
#include <oleg.h>
#include <logging.h>
}

#define B_FAILED_TO_OPEN 601

namespace kyotopantry {
	// The vault is responsible for storing duplicated blocks and hashes of
	// blocks that have been seen before.
	class vault {
	public:
		vault();
		~vault();

		// Main thread:
		void do_work();
	private:
		ol_database *block_db;
		std::thread vault_thread;
	};
}
