#pragma once

#include <string>

#define GK_FAILED_TO_OPEN 600

extern "C" {
#include <oleg.h>
}

namespace kyotopantry {
    class gatekeeper {
    public:
        gatekeeper();
        ~gatekeeper();

        bool queue_file_job(const std::string *path);

    private:
        ol_database *jobs_db;
    };
}
