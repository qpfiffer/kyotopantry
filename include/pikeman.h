#pragma once
#include <thread>

// Pikemen are responsible for processing individual files.
namespace kyotopantry {
	class pikeman {
	public:
		pikeman();
		~pikeman();

		void do_work();
	private:
		int get_thread_id();
		std::thread worker_thread;
	};
}
