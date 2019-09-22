#include "smart_local_free.h"

#include <cassert>


void local_free_deleter::operator()(HLOCAL const& ptr) const
{
	HLOCAL const freed = LocalFree(ptr);
	assert(freed == nullptr);
	(void)freed;
}
