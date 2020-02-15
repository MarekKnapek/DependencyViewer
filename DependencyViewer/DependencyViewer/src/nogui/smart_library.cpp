#include "smart_library.h"

#include "cassert_my.h"


void library_deleter::operator()(HMODULE const& library) const
{
	BOOL const freed = FreeLibrary(library);
	assert(freed != 0);
}

smart_library load_library(wchar_t const* const& library_name)
{
	return smart_library(LoadLibraryW(library_name));
}

function_ptr_t get_function_address(smart_library const& library, char const* const& function_name)
{
	static_assert(sizeof(function_ptr_t) == sizeof(decltype(GetProcAddress(HMODULE{}, LPCSTR{}))), "");
	static_assert(alignof(function_ptr_t) == alignof(decltype(GetProcAddress(HMODULE{}, LPCSTR{}))), "");
	return reinterpret_cast<function_ptr_t>(GetProcAddress(library.get(), function_name));
}
