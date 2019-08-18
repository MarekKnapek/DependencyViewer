#pragma once


#include "../nogui/pe.h"
#include "../nogui/memory_manager.h"

#include <string>
#include <vector>
#include <experimental/filesystem>


struct file_info;
typedef std::vector<file_info> file_infos;


namespace fs = std::experimental::filesystem;


struct file_info
{
	void* m_tree_item;
	file_info const* m_orig_instance;
	wstring const* m_file_path;
	bool m_is_32_bit;
	pe_import_table_info m_import_table;
	pe_export_table_info m_export_table;
	file_infos m_sub_file_infos;
	pe_resources_table_info m_resources_table;
};

struct main_type
{
	memory_manager m_mm;
	file_info m_fi;
	std::string m_tmpn;
	std::wstring m_tmpw;
	fs::path m_tmpp;
};

wstring const* get_not_found_string();

main_type process(std::wstring const& main_file_path);
