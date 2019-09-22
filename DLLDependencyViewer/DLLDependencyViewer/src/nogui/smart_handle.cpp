#include "smart_handle.h"

#include <cassert>

#include "my_windows.h"


void close_handle_deleter::operator()(void* const ptr) const
{
	BOOL const closed = CloseHandle(ptr);
	assert(closed != 0);
}
