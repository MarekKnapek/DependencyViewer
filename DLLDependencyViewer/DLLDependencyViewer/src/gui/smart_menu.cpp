#include "smart_menu.h"

#include <cassert>

#include <windows.h>


void smart_menu_deleter::operator()(void* const ptr) const
{
	static_assert(sizeof(void*) == sizeof(HMENU), "");
	BOOL const deleted = DestroyMenu(reinterpret_cast<HMENU>(ptr));
	assert(deleted != 0);
}
