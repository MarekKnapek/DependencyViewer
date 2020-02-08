#include "file_info_getters.h"

#include "processor.h"

#include <cassert>
#include <cstdint>


string_handle const& get_dll_name_no_path(file_info const* const& fi)
{
	assert(fi);
	assert(!fi->m_file_path);
	assert(fi->m_parent);
	file_info const* const parent_fi = fi->m_parent;
	auto const dll_idx_ = fi - parent_fi->m_fis;
	assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
	auto const dll_idx = static_cast<std::uint16_t>(dll_idx_);
	assert(dll_idx < parent_fi->m_import_table.m_normal_dll_count + parent_fi->m_import_table.m_delay_dll_count);
	auto const& dll_name = parent_fi->m_import_table.m_dll_names[dll_idx];
	assert(dll_name);
	return dll_name;
};
