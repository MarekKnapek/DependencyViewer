#pragma once


#include <memory>
#include <type_traits>

#include "my_windows.h"


struct local_free_deleter
{
	void operator()(HLOCAL const& ptr) const;
};
typedef std::unique_ptr<void, local_free_deleter> smart_local_free;
