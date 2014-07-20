#include <msgpack.hpp>
#include <zmq.hpp>
#include <thread>
#include "gatekeeper.h"
#include "pikeman.h"

using namespace kyotopantry;

gatekeeper::gatekeeper(bool verbose) {
    this->verbose = verbose;
    jobs_db = ol_open(".kyotopantry/", "jobs", OL_F_SPLAYTREE);

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
    scheduler_thread.join();
    ol_close(jobs_db);
}

void gatekeeper::get_jobs_from_db(std::vector<std::string> *jobs_list) {
    size_t datasize;
    unsigned char *job_results = NULL;
    // Pull jobs from db
    assert(ol_unjar_ds(jobs_db, JOBS_LIST, strlen(JOBS_LIST), &job_results, &datasize) == 0);

    msgpack::object obj;
    msgpack::unpacked unpacked;

    msgpack::unpack(&unpacked, (char *)job_results, datasize);
    obj = unpacked.get();
    obj.convert(jobs_list);

    free(job_results);
}

bool gatekeeper::queue_file_job(std::string &path) {
    std::vector<std::string> jobs_list;
    get_jobs_from_db(&jobs_list);

    // Actually add it to the thing in the DB
    jobs_list.push_back(path);

    // Save jobs back to db
    msgpack::sbuffer *to_save = new msgpack::sbuffer;
    msgpack::pack(to_save, jobs_list);

    int ret = ol_jar(jobs_db, JOBS_LIST, strlen(JOBS_LIST), (unsigned char *)to_save->data(), to_save->size());
    assert(ret == 0);

    return true;
}

std::string gatekeeper::get_next_job() {
    std::vector<std::string> jobs_list;
    get_jobs_from_db(&jobs_list);
    return "";
}

void gatekeeper::scheduler() {
    zmq::context_t context(2);
    zmq::socket_t socket(context, ZMQ_REP);
    zmq::socket_t main_loop_socket(context, ZMQ_REQ);
    socket.bind(SCHEDULER_URI);
    main_loop_socket.connect(MAINLOOP_URI);

    // Process job requests from pikemen:
    while(true) {
        std::map<std::string, std::string> resp;
        zmq::message_t request;
        if (verbose)
            ol_log_msg(LOG_INFO, "Scheduler waiting.");
        assert(socket.recv(&request) == true);

        msgpack::object obj;
        msgpack::unpacked unpacked;

        msgpack::unpack(&unpacked, (char *)request.data(), request.size());
        obj = unpacked.get();
        obj.convert(&resp);


        if (resp["type"] == "job_request") {
            if (verbose)
                ol_log_msg(LOG_INFO, "Scheduler receieved job request.");
            std::string next_file = get_next_job();

            zmq::message_t response(next_file.size());
            memcpy((void *)response.data(), next_file.data(), next_file.size());

            socket.send(response);
        } else if (resp["type"] == "job_finished") {
            if (verbose)
                ol_log_msg(LOG_INFO, "Scheduler receieved job finished.");
        } else if (resp["type"] == "shutdown") {
            break;
        }
    }
}

void gatekeeper::main_loop(bool verbose, int num_workers) {
    int i;

    // Prepare ZMQ stuff
    zmq::context_t context(2);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind(MAINLOOP_URI);

    // Spin up Pikemen
    kyotopantry::pikeman *pikemen[num_workers];
    for (i = 0; i < num_workers; i++) {
        kyotopantry::pikeman *new_recruit = new kyotopantry::pikeman();
        pikemen[i] = new_recruit;
    }

    // Wait for the scheduler to tell us that the job is done
    std::map<std::string, std::string> resp;

    // Receive from whoever
    zmq::message_t request;
    if (verbose)
        ol_log_msg(LOG_INFO, "Main loop waiting.");
    assert(socket.recv(&request) == true);

    msgpack::object obj;
    msgpack::unpacked unpacked;

    // Unpack and convert
    msgpack::unpack(&unpacked, (char *)request.data(), request.size());
    obj = unpacked.get();
    obj.convert(&resp);

    ol_log_msg(LOG_INFO, "Main loop receieved: %s", obj);

    for (i = 0; i < num_workers; i++) {
        kyotopantry::pikeman *recruit = pikemen[i];
        delete recruit;
    }
}
