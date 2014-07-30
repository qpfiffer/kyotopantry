// vim: noet ts=4 sw=4
#pragma once
#include <string>

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
	private:
		virtual bool do_job() = 0;
	protected:
		std::string current_file_name;
		size_t current_file_size;
		void *current_file;
	};

	class indexjob: public filejob {
	public:
		indexjob(std::string filename) : filejob(filename) {};
		~indexjob();
	private:
		bool do_job();
	};

	class dedupejob: public filejob {
	public:
		dedupejob(std::string filename) : filejob(filename) {};
		~dedupejob();
	private:
		bool do_job();
	}
}
