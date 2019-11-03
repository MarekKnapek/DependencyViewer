#pragma once


#include "../nogui/my_string_handle.h"

#include <filesystem>
#include <string>


struct main_type;


struct searcher
{
	main_type* m_mo;
	wstring_handle m_main_file_path;
};

void search(searcher& sch, string_handle const& dll_name);
