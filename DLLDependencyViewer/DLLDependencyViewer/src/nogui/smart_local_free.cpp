#include "smart_local_free.h"

#include <cassert>


void local_free_deleter::operator()(HLOCAL const& ptr) const
{
	[[maybe_unused]] HLOCAL const freed = LocalFree(ptr);
	assert(freed == nullptr);
}
