// vim: noet ts=4 sw=4
#include "vault.h"

using namespace kyotopantry;

vault::vault() {
	block_db = ol_open(".kyotopantry/", "jobs", OL_F_SPLAYTREE | OL_F_APPENDONLY);

	if (block_db == NULL)
		throw B_FAILED_TO_OPEN;

	vault_thread = std::thread(&vault::do_work, this);
}

vault::~vault() {
	ol_close(block_db);
}

void vault::do_work() {
}
