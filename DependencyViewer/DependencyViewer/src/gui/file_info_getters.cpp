#include "file_info_getters.h"

#include "processor.h"

#include "../nogui/cassert_my.h"
#include "../nogui/string_converter.h"
#include "../nogui/utils.h"


string_handle const& get_dll_name_no_path(file_info const* const& fi)
{
	assert(fi);
	assert(fi->m_parent);
	file_info const* const parent_fi = fi->m_parent;
	auto const dll_idx_ = fi - parent_fi->m_fis;
	assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
	std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
	assert(dll_idx < parent_fi->m_import_table.m_normal_dll_count + parent_fi->m_import_table.m_delay_dll_count);
	string_handle const& dll_name = parent_fi->m_import_table.m_dll_names[dll_idx];
	assert(dll_name);
	return dll_name;
}

bool compare_fi_by_path_or_name_caseinsensitive(file_info const* const& a, file_info const* const& b)
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


wstring get_tree_fi_name(file_info const* const& tmp_fi, string_converter& str_cnvrtr)
{
	assert(tmp_fi);
	file_info const* const parent_fi = tmp_fi->m_parent;
	if(parent_fi)
	{
		string_handle const& dll_name_a = get_dll_name_no_path(tmp_fi);
		wchar_t const* const dll_name_w = str_cnvrtr.convert(dll_name_a);
		assert(dll_name_w);
		wstring const ret{dll_name_w, size(dll_name_a)};
		return ret;
	}
	else
	{
		file_info const* const fi = tmp_fi->m_orig_instance ? tmp_fi->m_orig_instance : tmp_fi;
		assert(fi->m_file_path);
		wstring const file_name = find_file_name(fi->m_file_path);
		assert(file_name);
		return file_name;
	}
}

wstring get_tree_fi_path(file_info const* const& tmp_fi, string_converter& str_cnvrtr)
{
	assert(tmp_fi);
	file_info const* const fi = tmp_fi->m_orig_instance ? tmp_fi->m_orig_instance : tmp_fi;
	if(fi->m_file_path)
	{
		wstring const& ret = *fi->m_file_path.m_string;
		return ret;
	}
	else
	{
		wstring const ret = get_tree_fi_name(tmp_fi, str_cnvrtr);
		return ret;
	}
}


wstring get_modules_fi_name(file_info const* const& fi, string_converter& str_cnvrtr)
{
	assert(fi);
	assert(!fi->m_orig_instance);
	if(fi->m_file_path)
	{
		wstring const file_name = find_file_name(fi->m_file_path);
		assert(file_name);
		return file_name;
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

wstring get_modules_fi_path(file_info const* const& fi)
{
	assert(fi);
	assert(!fi->m_orig_instance);
	if(fi->m_file_path)
	{
		wstring const& ret = *fi->m_file_path.m_string;
		return ret;
	}
	else
	{
		static constexpr wchar_t const s_modules_path_not_found[] = L"";
		static constexpr int const s_modules_path_not_found_len = 0;
		wstring const ret{s_modules_path_not_found, s_modules_path_not_found_len};
		return ret;
	}
}
