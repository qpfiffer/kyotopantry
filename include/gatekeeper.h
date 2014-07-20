#pragma once

#include <chrono>
#include <string>
#include <thread>

#define GK_FAILED_TO_OPEN 600

#define MAINLOOP_URI "ipc://.mainloop.sock"
#define SCHEDULER_URI "ipc://.scheduler.sock"

#define JOBS_LIST "all_jobs"

extern "C" {
#include <oleg.h>
#include <logging.h>
}

namespace kyotopantry {
    class gatekeeper {
    public:
        gatekeeper(bool verbose);
        ~gatekeeper();

        bool queue_file_job(std::string &path);
        void scheduler();
        void main_loop(bool verbose, int num_workers);
    private:
        bool verbose;
        ol_database *jobs_db;
        std::thread scheduler_thread;
    };
}
