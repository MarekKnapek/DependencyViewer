#include "dbg.h"

#include "dbg_provider.h"

#include <cassert>


static dbg_provider* s_dbg_provider = nullptr;


void dbg_start()
{
	assert(!s_dbg_provider);
	s_dbg_provider = new dbg_provider;
}

void dbg_stop()
{
	assert(s_dbg_provider);
	delete s_dbg_provider;
	s_dbg_provider = nullptr;
}

void dbg_get_symbols_from_addresses(get_symbols_from_addresses_task_t* const param)
{
	assert(s_dbg_provider);
	s_dbg_provider->get_symbols_from_addresses(param);
}
