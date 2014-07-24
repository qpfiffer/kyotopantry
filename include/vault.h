// vim: noet ts=4 sw=4
#pragma once

extern "C" {
#include <oleg.h>
#include <logging.h>
}

namespace kyotopantry {
    // The vault is responsible for storing duplicated blocks and hashes of
    // blocks that have been seen before.
    class vault {
    public:
        vault();
        ~vault();
    private:
        ol_database *blockdb;
    };
}
