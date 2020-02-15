#include "smart_reg_key.h"

#include "cassert_my.h"


void reg_close_key_deleter::operator()(void* const key) const
{
	static_assert(sizeof(void*) == sizeof(HKEY), "");
	LSTATUS const closed = RegCloseKey(reinterpret_cast<HKEY>(key));
	assert(closed == ERROR_SUCCESS);
}

smart_reg_key make_smart_reg_key(HKEY const key)
{
	static_assert(sizeof(HKEY) == sizeof(void*), "");
	return smart_reg_key(reinterpret_cast<void*>(key));
}
