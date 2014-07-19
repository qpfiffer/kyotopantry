#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>

// Gank some stuff from Oleg:
extern "C" {
#include <oleg.h>
#include <logging.h>
}

#include "gatekeeper.h"

bool file_exists(const char *path) {
    int fd;
    fd = open(path, O_RDONLY);

    if (fd < 0) {
        errno = 0;
        return false;
    }
    close(fd);

    return true;
}

int main(int argc, char *argv[]) {
    // Main gatekeeper:
    kyotopantry::gatekeeper mainKeeper;
    bool verbose = false;


    // Look for any arguments:
    int i = 1, files_start_at = 1;;
    for (i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg[0] == '-') {
            std::string cmd = arg.substr(1);

            if (cmd == "v" || cmd == "-verbose") {
                verbose = true;
            }
        } else {
            // This isn't something that starts with a -. Assume it is a file
            // and move on.
            files_start_at = i + 1;
            break;
        }
    }

    if (argc < 2 || ((argc-1) - files_start_at <= 0)) {
        ol_log_msg(LOG_ERR, "You need to specify some directories/files to work on.");
        return 1;
    }
    ol_log_msg(LOG_INFO, "Starting kyotopantry.");


    // Process files:
    int files_added = 0;
    for (i = files_start_at; i < argc; i++) {
        const std::string file_to_add = argv[i];

        if (!file_exists(file_to_add.c_str())) {
            ol_log_msg(LOG_WARN, "File %s doesn't exist or I can't open it or something.", argv[i]);
            continue;
        }

        if (verbose)
            ol_log_msg(LOG_INFO, "Adding %s to queue...", file_to_add.c_str());

        if (!mainKeeper.queue_file_job(&file_to_add)) {
            ol_log_msg(LOG_ERR, "Could not add file to queue.");
            return 1;
        }
        files_added++;
    }

    if (files_added == 0) {
        ol_log_msg(LOG_ERR, "Could not add any files. Bummer.");
        return 1;
    }

    if (verbose)
        ol_log_msg(LOG_INFO, "Processing %i files...", files_added);

    return 0;
}
