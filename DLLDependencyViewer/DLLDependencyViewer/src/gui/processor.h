#pragma once

#include "../nogui/memory_manager.h"
#include "../nogui/my_string_handle.h"
#include "../nogui/pe.h"

#include <cstdint>


struct file_info
{
	void* m_tree_item;
	file_info* m_fis;
	file_info* m_orig_instance;
	wstring_handle m_file_path;
	bool m_is_32_bit;
	pe_import_table_info m_import_table;
	pe_export_table_info m_export_table;
	std::uint16_t* m_matched_imports;
};
void init(file_info* const fi);
void init(file_info* const fi, int const count);

struct main_type
{
	file_info m_fi;
	memory_manager m_mm;
};


bool process(std::vector<std::wstring> const& file_paths, main_type* const mo_out);
