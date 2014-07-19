#include "gatekeeper.h"

using namespace kyotopantry;

gatekeeper::gatekeeper() {
    jobs_db = ol_open(".kyotopantry/", "jobs", OL_F_APPENDONLY | OL_F_SPLAYTREE);

    if (jobs_db == NULL) {
        throw GK_FAILED_TO_OPEN;
    }
}

gatekeeper::~gatekeeper() {
    ol_close(jobs_db);
}

bool gatekeeper::queue_file_job(const std::string *path) {
    return true;
}
