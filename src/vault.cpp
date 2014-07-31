// vim: noet ts=4 sw=4
#include "gatekeeper.h"
#include "vault.h"

using namespace kyotopantry;

vault::vault() {
	block_db = ol_open(".kyotopantry/", "blocks", OL_F_SPLAYTREE | OL_F_APPENDONLY);

	if (block_db == NULL)
		throw B_FAILED_TO_OPEN;

	this->context = new zmq::context_t(1);
	this->socket = new zmq::socket_t(*context, ZMQ_REP);
	socket->bind(VAULT_URI);

	vault_thread = std::thread(&vault::do_work, this);
}

vault::~vault() {
	ol_close(block_db);

	delete socket;
	delete context;
}

void vault::spin() {
	vault_thread.join();
}

void vault::do_work() {
	while (true) {
		SchedulerMessage resp;

		zmq::message_t request;
		ol_log_msg(LOG_INFO, "Vault waiting.");
		assert(socket->recv(&request) == true);

		ol_log_msg(LOG_INFO, "Vault receieved message.");

		msgpack::object obj;
		msgpack::unpacked unpacked;

		msgpack::unpack(&unpacked, (char *)request.data(), request.size());
		obj = unpacked.get();
		obj.convert(&resp);

		if (resp["type"] == "shutdown") {
			break;
		}
	}
}
