// vim: noet ts=4 sw=4
#pragma once
#include <string>

// This is the size of the block we extract as we
// iterate through a file.
#define DEFAULT_BLOCKSIZE 4096

namespace kyotopantry {
	class filejob {
	public:
		filejob(std::string filename);

		virtual std::string get_current_file_name() {
			return current_file_name;
		}
		virtual size_t get_current_file_size() {
			return current_file_size;
		}

		virtual bool do_job() = 0;

	protected:
		std::string current_file_name;
		size_t current_file_size;
		void *current_file;
	};

	// This job looks through all the chunks in a file, hashes them
	// and sends them off to the vault.
	class indexjob: public filejob {
	public:
		indexjob(std::string filename) : filejob(filename) {};
		~indexjob();
		bool do_job();
	};

	// This job is repsonsible for copying files. It removes any blocks that
	// are known to exist by asking the vault about them.
	class dedupejob: public filejob {
	public:
		dedupejob(std::string filename) : filejob(filename) {};
		~dedupejob();
		bool do_job();
	};
}
