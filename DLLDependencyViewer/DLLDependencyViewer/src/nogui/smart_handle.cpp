#include "smart_handle.h"

#include <cassert>

#include <windows.h>


void close_handle_deleter::operator()(void* const ptr) const
{
	BOOL const closed = CloseHandle(ptr);
	assert(closed != 0);
}
