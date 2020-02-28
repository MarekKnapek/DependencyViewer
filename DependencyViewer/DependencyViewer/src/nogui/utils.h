#pragma once


#include "my_string.h"
#include "my_string_handle.h"


template<typename begin_it, typename end_it, typename val_t>
end_it rfind(begin_it const& begin, end_it const& end, val_t const& val);
wchar_t const* find_file_name(wchar_t const* const& file_path, int const& len);
wstring find_file_name(wstring const& file_path);
wstring find_file_name(wstring_handle const& file_path);
