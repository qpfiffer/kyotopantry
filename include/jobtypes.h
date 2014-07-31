// vim: noet ts=4 sw=4
#pragma once
#include <string>

// This is the size of the block we extract as we
// iterate through a file.
#define DEFAULT_BLOCKSIZE 4096

// Size of BMW hash
#define HASH_SIZE 512

// Controls how long a "try again" job will sleep for
#define SNOOZE_AMT 128

namespace kyotopantry {
	class filejob {
	public:
		filejob(std::string filename, const int job_id);

		virtual std::string get_current_file_name() {
			return current_file_name;
		}
		virtual size_t get_current_file_size() {
			return current_file_size;
		}

		virtual bool do_job() = 0;

		const int job_id;
	protected:
		std::string current_file_name;
		size_t current_file_size;
		void *current_file;
	};

	// This job looks through all the chunks in a file, hashes them
	// and sends them off to the vault.
	class indexjob: public filejob {
	public:
		indexjob(std::string filename, const int id) : filejob(filename, id) {};
		~indexjob();
		bool do_job();
	private:
		bool setup_map();
		bool hash_blocks();
	};

	// This job is repsonsible for copying files. It removes any blocks that
	// are known to exist by asking the vault about them.
	class dedupejob: public filejob {
	public:
		dedupejob(std::string filename, const int id) : filejob(filename, id) {};
		~dedupejob();
		bool do_job();
	};

	// This jobs purpose is to just sleep a thread for a little bit,
	// while we wait for index jobs to stop happening.
	class sleepjob: public filejob {
	public:
		sleepjob(std::string filename, const int id) : filejob(filename, id) {};
		bool do_job();
	};
}
