#include "file_info_getters.h"

#include "processor.h"

#include "../nogui/cassert_my.h"
#include "../nogui/string_converter.h"
#include "../nogui/utils.h"


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
}

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
}


wstring get_modules_list_col_name(modules_list_t const& modlist, std::uint16_t const& idx, string_converter& str_cnvrtr)
{
	assert(idx < modlist.m_count);
	file_info const* const fi = modlist.m_list[idx];
	assert(!fi->m_orig_instance);
	if(fi->m_file_path)
	{
		wchar_t const* const name = find_file_name(begin(fi->m_file_path), size(fi->m_file_path));
		assert(name != begin(fi->m_file_path));
		int const len = size(fi->m_file_path) - static_cast<int>(name - begin(fi->m_file_path));
		wstring const ret{name, len};
		return ret;
	}
	else
	{
		string_handle const& dll_name_a = get_dll_name_no_path(fi);
		wchar_t const* const dll_name_w = str_cnvrtr.convert(dll_name_a);
		assert(dll_name_w);
		wstring const ret{dll_name_w, size(dll_name_a)};
		return ret;
	}
}

wstring get_modules_list_col_path(modules_list_t const& modlist, std::uint16_t const& idx)
{
	assert(idx < modlist.m_count);
	file_info const* const fi = modlist.m_list[idx];
	assert(!fi->m_orig_instance);
	if(fi->m_file_path)
	{
		wstring const& ret = *fi->m_file_path.m_string;
		return ret;
	}
	else
	{
		wstring const ret{L"", 0};
		return ret;
	}
}
