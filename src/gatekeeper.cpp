#include <msgpack.hpp>
#include <zmq.hpp>
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
    zmq::context_t context(2);
    zmq::socket_t socket(context, ZMQ_REP);
    zmq::socket_t main_loop_socket(context, ZMQ_REP);
    socket.bind(SCHEDULER_URI);
    main_loop_socket.bind(MAINLOOP_URI);

    // Process job requests from pikemen:
    while(true) {
        zmq::message_t request;
        assert(socket.recv(&request) == true);

        msgpack::object obj;
        msgpack::unpacked unpacked;

        msgpack::unpack(&unpacked, (char *)request.data(), request.size());
        obj = unpacked.get();

        ol_log_msg(LOG_INFO, "Scheduler receieved: %s", obj);

        main_loop_socket.send(request);
        zmq::message_t request_2;
        assert(scheduler_socket.recv(&request_2) == true);

        // Send the response from the scheduler back to python
        socket.send(request_2);
    }
}

void gatekeeper::main_loop(bool verbose, int num_workers) {
    int i;

    // Prepare ZMQ stuff
    zmq::context_t context(2);
    zmq::socket_t socket(context, ZMQ_REP);
    zmq::socket_t scheduler_socket(context, ZMQ_REQ);
    socket.bind(MAINLOOP_URI);
    scheduler_socket.bind(MAINLOOP_URI);

    // Spin up Pikemen
    kyotopantry::pikeman *pikemen[num_workers];
    for (i = 0; i < num_workers; i++) {
        kyotopantry::pikeman *new_recruit = new kyotopantry::pikeman();
        pikemen[i] = new_recruit;
    }

    // Wait for the scheduler to tell us that the job is done
    while(true) {
        zmq::message_t request;
        assert(socket.recv(&request) == true);

        msgpack::object obj;
        msgpack::unpacked unpacked;

        msgpack::unpack(&unpacked, (char *)request.data(), request.size());
        obj = unpacked.get();

        ol_log_msg(LOG_INFO, "Main loop receieved: %s", obj);
        break;
    }

    for (i = 0; i < num_workers; i++) {
        kyotopantry::pikeman *recruit = pikemen[i];
        delete recruit;
    }
}
