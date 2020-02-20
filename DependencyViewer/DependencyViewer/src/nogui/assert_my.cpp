#include "assert_my.h"

#include "windows_my.h"


void assert_function(wchar_t const* const& str)
{
	OutputDebugStringW(str);
}
