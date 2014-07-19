#include <thread>
#include "gatekeeper.h"
#include "pikeman.h"

using namespace kyotopantry;

gatekeeper::gatekeeper() {
    jobs_db = ol_open(".kyotopantry/", "jobs", OL_F_APPENDONLY | OL_F_SPLAYTREE);

    if (jobs_db == NULL) {
        throw GK_FAILED_TO_OPEN;
    }

    scheduler_thread = std::thread(&gatekeeper::scheduler, this);
}

gatekeeper::~gatekeeper() {
    ol_close(jobs_db);
    scheduler_thread.join();
}

bool gatekeeper::queue_file_job(const std::string *path) {
    return true;
}

void gatekeeper::scheduler() {
}

void gatekeeper::main_loop(bool verbose, int num_workers) {
    int i;

    kyotopantry::pikeman *pikemen[num_workers];
    for (i = 0; i < num_workers; i++) {
        kyotopantry::pikeman *new_recruit = new kyotopantry::pikeman();
        pikemen[i] = new_recruit;
    }

    for (i = 0; i < num_workers; i++) {
        kyotopantry::pikeman *recruit = pikemen[i];
        delete recruit;
    }
}
