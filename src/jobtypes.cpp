// vim: noet ts=4 sw=4
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <zmq.hpp>

#include "jobtypes.h"
#include "pikeman.h"
#include "gatekeeper.h"
#include "SHA3api_ref.h"

using namespace kyotopantry;

filejob::filejob(std::string filename, const int id):
	job_id(id), current_file_name(filename), current_file_size(0), current_file(NULL) {
}

indexjob::~indexjob() {
	munmap(current_file, current_file_size);
}

dedupejob::~dedupejob() {
}

bool indexjob::do_job() {
	struct stat sb = {0};
	int fd = open(this->current_file_name.c_str(), O_RDONLY);

	if (fd <= 0) {
		//ol_log_msg(LOG_WARN, "%s: Could not open file %s.",
		//		thread_name.c_str(), this->current_file_name.c_str());
		errno = 0;
		return false;
	}

	if (fstat(fd, &sb) == -1) {
		//ol_log_msg(LOG_WARN, "%s: Could not open file %s.",
		//		thread_name.c_str(), this->current_file_name.c_str());
		errno = 0;
		close(fd);
		return false;
	}

	current_file_size = sb.st_size;

	ol_log_msg(LOG_INFO, "Working on file of size %i.", current_file_size);

	current_file = mmap(NULL, current_file_size, PROT_READ, MAP_SHARED, fd, 0);
	if (current_file == MAP_FAILED) {
		ol_log_msg(LOG_WARN, "Could not mmap file.");
		close(fd);
		return false;
	}


	unsigned int i = 0;
	for (; i < (current_file_size / DEFAULT_BLOCKSIZE) + 1; i++) {
		const unsigned int possible_chunk_end = (i * DEFAULT_BLOCKSIZE) + DEFAULT_BLOCKSIZE;
		const unsigned int chunk_start = i * DEFAULT_BLOCKSIZE;
		const unsigned int chunk_end = possible_chunk_end > current_file_size ?
			current_file_size : possible_chunk_end;

		ol_log_msg(LOG_INFO, "Hashing %i bytes of file.", chunk_end - chunk_start);

		// We hash each DEFAULT_BLOCKSIZE block individually
		unsigned char *data_ptr = (unsigned char *)this->current_file + chunk_start;
		unsigned char hash[64] = {0};

		if (Hash(HASH_SIZE, data_ptr, chunk_end, hash) != 0) {
			ol_log_msg(LOG_WARN, "Could not update Blue Midnight Wish.");
			return false;
		}

		char buf[128] = {0};
		int j = 0;
		for (j=0; j<(HASH_SIZE/8); j++)
			sprintf(buf + (j * 2), "%02X", hash[j]);

		ol_log_msg(LOG_INFO, "Hash of filechunk is %s.", buf);
	}

	ol_log_msg(LOG_INFO, "Hashed %i/%i chunks.", i, (current_file_size / DEFAULT_BLOCKSIZE) + 1);

	close(fd);
	return true;
}

bool dedupejob::do_job() {
	return true;
}

bool sleepjob::do_job() {
	std::this_thread::sleep_for(std::chrono::milliseconds(SNOOZE_AMT));
	return true;
}
