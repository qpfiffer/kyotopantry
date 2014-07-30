// vim: noet ts=4 sw=4
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <zmq.hpp>
#include "jobtypes.h"
#include "pikeman.h"
#include "gatekeeper.h"

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
