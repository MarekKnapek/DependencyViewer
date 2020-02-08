#pragma once


#include <memory>


struct smart_menu_deleter
{
	void operator()(void* const ptr) const;
};

typedef std::unique_ptr<void, smart_menu_deleter> smart_menu;
