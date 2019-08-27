#include "assert.h"

#include <windows.h>


void assert_function(wchar_t const* const& str)
{
	OutputDebugStringW(str);
}
