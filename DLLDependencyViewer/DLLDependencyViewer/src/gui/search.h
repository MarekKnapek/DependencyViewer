#pragma once


#include <string>


struct searcher
{
	std::wstring const* m_main_file_path;
};

std::wstring search(searcher& sch, std::wstring const& dll_name);
