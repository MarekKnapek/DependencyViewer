#pragma once


#include <string>
#include <experimental/filesystem>


template<typename> struct basic_string;
typedef basic_string<char> string;
typedef basic_string<wchar_t> wstring;
struct main_type;


namespace fs = std::experimental::filesystem;


struct searcher
{
	main_type* m_mo;
	std::wstring const* m_main_file_path;
};

void search(searcher& sch, string const* const& dll_name);
