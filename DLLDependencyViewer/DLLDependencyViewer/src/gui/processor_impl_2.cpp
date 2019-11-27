#include "processor_impl_2.h"

#include "processor_2.h"

#include "../nogui/assert.h"

#include <algorithm>
#include <cstdint>
#include <iterator>


static constexpr wchar_t const s_dummy_textw_r[] = L"";
static constexpr wstring const s_dummy_textw_s = {s_dummy_textw_r, static_cast<int>(std::size(s_dummy_textw_r)) - 1};
static constexpr wstring_handle const s_dummy_textw_h = {&s_dummy_textw_s};
static constexpr char const s_dummy_texta_r[] = "";
static constexpr string const s_dummy_texta_s ={s_dummy_texta_r, static_cast<int>(std::size(s_dummy_texta_r)) - 1};
static constexpr string_handle const s_dummy_texta_h = {&s_dummy_texta_s};


bool process_impl_2(std::vector<std::wstring> const& file_paths, file_info_2& fi, memory_manager& mm)
{
	WARN_M_R(file_paths.size() < 0xFFFF, L"Too many files to process.", false);
	std::uint16_t const n = static_cast<std::uint16_t>(file_paths.size());
	file_info_2* const fis = mm.m_alc.allocate_objects<file_info_2>(n);
	init(fis, n);
	string_handle* const dll_names = mm.m_alc.allocate_objects<string_handle>(n);
	std::fill(dll_names, dll_names + n, s_dummy_texta_h);
	std::uint16_t* const import_counts = mm.m_alc.allocate_objects<std::uint16_t>(n);
	std::fill(import_counts, import_counts + n, std::uint16_t{0});
	init(&fi);
	fi.m_fis = fis;
	fi.m_file_path = s_dummy_textw_h;
	fi.m_import_table.m_dll_count = n;
	fi.m_import_table.m_non_delay_dll_count = n;
	fi.m_import_table.m_dll_names = dll_names;
	fi.m_import_table.m_import_counts = import_counts;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info_2& sub_fi = fi.m_fis[i];
		int const path_len = static_cast<int>(file_paths[i].size());
		wchar_t const* const cstr = file_paths[i].c_str();
		wstring_handle const file_path = mm.m_wstrs.add_string(cstr, path_len, mm.m_alc);
		sub_fi.m_file_path = file_path;
		step_1(file_path, sub_fi, mm);
	}
	return true;
}


void step_1(wstring_handle const& origin, file_info_2& fi, memory_manager& mm)
{
}
