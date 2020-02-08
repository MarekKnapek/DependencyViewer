#pragma once


#include <memory>

#include "my_windows.h"


struct reg_close_key_deleter
{
public:
	void operator()(void* const key) const;
};
typedef std::unique_ptr<void, reg_close_key_deleter> smart_reg_key;

smart_reg_key make_smart_reg_key(HKEY const key);
