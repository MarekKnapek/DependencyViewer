#include "file_info_getters.h"

#include "processor.h"

#include "../nogui/cassert_my.h"

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

bool compare_fi_by_path_or_name(file_info const* const& a, file_info const* const& b)
{
	assert(a);
	assert(b);
	assert(!a->m_orig_instance);
	assert(!b->m_orig_instance);
	auto const& path_a = a->m_file_path;
	auto const& path_b = b->m_file_path;
	if(path_a && path_b)
	{
		auto const ret = wstring_handle_case_insensitive_less{}(path_a, path_b);
		return ret;
	}
	else if(!path_a && !path_b)
	{
		auto const& dll_name_a = get_dll_name_no_path(a);
		auto const& dll_name_b = get_dll_name_no_path(b);
		auto const ret = string_handle_case_insensitive_less{}(dll_name_a, dll_name_b);
		return ret;
	}
	else if(path_a && !path_b)
	{
		return false;
	}
	else
	{
		assert(!path_a && path_b);
		return true;
	}
};
