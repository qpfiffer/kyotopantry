// vim: noet ts=4 sw=4
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
}
