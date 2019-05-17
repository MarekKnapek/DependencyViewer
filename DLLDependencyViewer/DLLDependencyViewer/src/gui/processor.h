#pragma once


#include "../nogui/pe.h"

#include <string>
#include <vector>


struct file_info;
typedef std::vector<file_info> file_infos;


struct file_info
{
	std::wstring m_file_name;
	std::wstring m_file_path;
	pe_import_table_info m_import_table;
	pe_export_table_info m_export_table;
	file_info* m_orig_instance;
	file_infos m_sub_file_infos;
};


file_info process(std::wstring const& main_file_path);
