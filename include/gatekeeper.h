#pragma once

#include <chrono>
#include <string>
#include <thread>

#define GK_FAILED_TO_OPEN 600

extern "C" {
#include <oleg.h>
#include <logging.h>
}

namespace kyotopantry {
    class gatekeeper {
    public:
        gatekeeper();
        ~gatekeeper();

        bool queue_file_job(const std::string *path);
        void scheduler();
        void main_loop(bool verbose, int num_workers);
    private:
        ol_database *jobs_db;
        std::thread scheduler_thread;
    };
}
