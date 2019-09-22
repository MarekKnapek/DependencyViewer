#pragma once


#include <memory>

#include "my_windows.h"
#include <unknwn.h>


struct com_interface_deleter
{
	void operator()(IUnknown* const& ptr) const;
};
using smart_com_interface = std::unique_ptr<IUnknown, com_interface_deleter>;
