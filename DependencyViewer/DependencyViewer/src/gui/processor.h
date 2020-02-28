#pragma once

#include "../nogui/memory_manager.h"
#include "../nogui/my_string_handle.h"
#include "../nogui/pe.h"

#include <cstdint>


struct htreeitem_t;
typedef htreeitem_t* htreeitem;


struct file_info
{
	htreeitem m_tree_item;
	file_info* m_fis;
	file_info* m_parent;
	file_info* m_orig_instance;
	file_info* m_prev_instance;
	file_info* m_next_instance;
	wstring_handle m_file_path;
	pe_import_table_info m_import_table;
	pe_export_table_info m_export_table;
	std::uint16_t* m_matched_imports;
	std::uint8_t m_icon;
	bool m_is_32_bit;
};
void init(file_info* const fi);
void init(file_info* const fi, int const count);

struct modules_list_t
{
	file_info** m_list;
	std::uint16_t m_count;
};

struct main_type
{
	file_info* m_fi;
	modules_list_t m_modules_list;
	memory_manager m_mm;
	void swap(main_type& other) noexcept;
};
inline void swap(main_type& a, main_type& b) noexcept { a.swap(b); }


bool process(std::vector<std::wstring> const& file_paths, main_type* const mo_out);
