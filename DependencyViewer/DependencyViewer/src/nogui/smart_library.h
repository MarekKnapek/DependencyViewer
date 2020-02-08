#pragma once


#include <memory>
#include <type_traits>

#include "my_windows.h"


struct library_deleter;


using HMODULE_value_type = std::remove_reference_t<decltype(*(HMODULE{}))>;
using function_ptr_t = void(*)();
using smart_library = std::unique_ptr<HMODULE_value_type, library_deleter>;


struct library_deleter
{
	void operator()(HMODULE const& library) const;
};


smart_library load_library(wchar_t const* const& library_name);
function_ptr_t get_function_address(smart_library const& library, char const* const& function_name);
