#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>

// Gank some stuff from Oleg:
extern "C" {
#include <oleg.h>
#include <logging.h>
}

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
    if (argc < 2) {
        ol_log_msg(LOG_ERR, "You need to specify some directories/files to work on.");
        return 1;
    }
    ol_log_msg(LOG_INFO, "Starting kyotopantry.");

    int i = 1;
    for (;i < argc; i++) {
        const char *file_to_add = argv[i];

        if (!file_exists(file_to_add)) {
            ol_log_msg(LOG_WARN, "File %s doesn't exist or I can't open it or something.", argv[i]);
            continue;
        }

        ol_log_msg(LOG_INFO, "Adding %s to queue...", argv[i]);
    }

    return 0;
}
