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

    std::vector<std::string> jobs_list;

    msgpack::sbuffer *to_save = new msgpack::sbuffer;
    msgpack::pack(to_save, jobs_list);

    // TODO: See if we have unprocessed jobs.
    int ret = ol_jar(jobs_db, JOBS_LIST, strlen(JOBS_LIST), (unsigned char *)to_save->data(), to_save->size());
    assert(ret == 0);

    scheduler_thread = std::thread(&gatekeeper::scheduler, this);
}

gatekeeper::~gatekeeper() {
    ol_close(jobs_db);
    scheduler_thread.join();
}

bool gatekeeper::queue_file_job(std::string &path) {
    size_t datasize;
    unsigned char *job_results = NULL;
    std::vector<std::string> jobs_list;
    // Pull jobs from db
    assert(ol_unjar_ds(jobs_db, JOBS_LIST, strlen(JOBS_LIST), &job_results, &datasize) == 0);

    msgpack::object obj;
    msgpack::unpacked unpacked;

    msgpack::unpack(&unpacked, (char *)job_results, datasize);
    obj = unpacked.get();
    obj.convert(&jobs_list);

    // Actually add it to the thing in the DB
    jobs_list.push_back(path);

    // Save jobs back to db
    msgpack::sbuffer *to_save = new msgpack::sbuffer;
    msgpack::pack(to_save, jobs_list);

    int ret = ol_jar(jobs_db, JOBS_LIST, strlen(JOBS_LIST), (unsigned char *)to_save->data(), to_save->size());
    assert(ret == 0);

    free(job_results);
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
        std::map<std::string, std::string> resp;
        zmq::message_t request;
        assert(socket.recv(&request) == true);

        msgpack::object obj;
        msgpack::unpacked unpacked;

        msgpack::unpack(&unpacked, (char *)request.data(), request.size());
        obj = unpacked.get();
        obj.convert(&resp);

        ol_log_msg(LOG_INFO, "Scheduler receieved: %s", obj);

        if (resp["type"] == "job_request") {
        } else if (resp["type"] == "job_finished") {
        }
        main_loop_socket.send(request);
        zmq::message_t request_2;
        assert(main_loop_socket.recv(&request_2) == true);

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
        std::map<std::string, std::string> resp;

        // Receive from whoever
        zmq::message_t request;
        assert(socket.recv(&request) == true);

        msgpack::object obj;
        msgpack::unpacked unpacked;

        // Unpack and convert
        msgpack::unpack(&unpacked, (char *)request.data(), request.size());
        obj = unpacked.get();
        obj.convert(&resp);

        ol_log_msg(LOG_INFO, "Main loop receieved: %s", obj);
        break;
    }

    for (i = 0; i < num_workers; i++) {
        kyotopantry::pikeman *recruit = pikemen[i];
        delete recruit;
    }
}
