#pragma once


#include <filesystem>
#include <string>


template<typename> struct basic_string;
typedef basic_string<char> string;
typedef basic_string<wchar_t> wstring;
struct main_type;


struct searcher
{
	main_type* m_mo;
	wstring const* m_main_file_path;
};

void search(searcher& sch, string const* const& dll_name);
