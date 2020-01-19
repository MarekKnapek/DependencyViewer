#include "processor_impl.h"

#include "import_export_matcher.h"
#include "processor.h"

#include "../nogui/act_ctx.h"
#include "../nogui/assert.h"
#include "../nogui/dependency_locator.h"
#include "../nogui/file_name_provider.h"
#include "../nogui/memory_mapped_file.h"
#include "../nogui/my_actctx.h"
#include "../nogui/pe2.h"
#include "../nogui/scope_exit.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>


static constexpr wchar_t const s_dummy_textw_r[] = L"";
static constexpr wstring const s_dummy_textw_s = {s_dummy_textw_r, static_cast<int>(std::size(s_dummy_textw_r)) - 1};
static constexpr wstring_handle const s_dummy_textw_h = {&s_dummy_textw_s};
static constexpr char const s_dummy_texta_r[] = "";
static constexpr string const s_dummy_texta_s = {s_dummy_texta_r, static_cast<int>(std::size(s_dummy_texta_r)) - 1};
static constexpr string_handle const s_dummy_texta_h = {&s_dummy_texta_s};


bool process_impl(std::vector<std::wstring> const& file_paths, file_info& fi, memory_manager& mm)
{
	my_actctx::deactivate();
	auto const activate_my_actctx = mk::make_scope_exit([](){ my_actctx::activate(); });
	WARN_M_R(file_paths.size() < 0xFFFF, L"Too many files to process.", false);
	std::uint16_t const n = static_cast<std::uint16_t>(file_paths.size());
	file_info* const fis = mm.m_alc.allocate_objects<file_info>(n);
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
	allocator tmpalc;
	tmp_type to;
	to.m_mm = &mm;
	to.m_tmp_alc = &tmpalc;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& sub_fi = fi.m_fis[i];
		int const path_len = static_cast<int>(file_paths[i].size());
		wchar_t const* const cstr = file_paths[i].c_str();
		wstring_handle const normalized = file_name_provider::get_correct_file_name(cstr, path_len, to.m_mm->m_wstrs, to.m_mm->m_alc);
		dependency_locator& dl = to.m_dl;
		dl.m_main_path = normalized;
		assert(to.m_queue.empty());
		to.m_queue.push_back({normalized, &sub_fi});
		bool const step = step_1(to);
		WARN_M_R(step, L"Failed to step_1.", false);
	}
	pair_root(fi, to);
	return true;
}


bool step_1(tmp_type& to)
{
	while(!to.m_queue.empty())
	{
		auto const& e = to.m_queue.front();
		wstring_handle const file_path = e.first;
		file_info* const fi = e.second;
		assert(fi != nullptr);
		to.m_queue.pop_front();
		bool const step = step_2(file_path, *fi, to);
		WARN_M_R(step, L"Failed to step_2.", false);
	}
	return true;
}

bool step_2(wstring_handle const& file_path, file_info& fi, tmp_type& to)
{
	auto const it = to.m_map.find(file_path);
	if(it != to.m_map.end())
	{
		assert(it->second->m_orig_instance);
		fi.m_orig_instance = it->second->m_orig_instance;
		return true;
	}
	fi.m_file_path = file_path;
	std::uint16_t const* enpt;
	std::uint16_t enpt_count;
	pe_tables tables;
	tables.m_tmp_alc = to.m_tmp_alc;
	tables.m_iti_out = &fi.m_import_table;
	tables.m_eti_out = &fi.m_export_table;
	tables.m_enpt_count_out = &enpt_count;
	tables.m_enpt_out = &enpt;
	{
		memory_mapped_file const mmf = memory_mapped_file(file_path.m_string->m_str);
		WARN_M_R(mmf.begin() != nullptr, L"Failed to memory_mapped_file.", false);
		bool const tables_processed = pe_process_all(mmf.begin(), mmf.size(), *to.m_mm, &tables);
		WARN_M_R(tables_processed, L"Failed to pe_process_all.", false);
	}
	assert(to.m_map.find(file_path) == to.m_map.end());
	fi.m_is_32_bit = tables.m_is_32_bit;
	fat_type* const fo = to.m_tmp_alc->allocate_objects<fat_type>(1);
	fo->m_orig_instance = &fi;
	fo->m_enpt.m_table = enpt;
	fo->m_enpt.m_count = enpt_count;
	to.m_map[file_path] = fo;
	std::uint16_t const n = fi.m_import_table.m_dll_count;
	file_info* const fis = to.m_mm->m_alc.allocate_objects<file_info>(n);
	init(fis, n);
	fi.m_fis = fis;
	dependency_locator& dl = to.m_dl;
	actctx_state_t actctx_state;
	bool const actctx_created = create_actctx(dl.m_main_path, fi.m_file_path, tables.m_manifest_id, &actctx_state);
	WARN_M_R(actctx_created, L"Failed to create_actctx.", false);
	auto const fn_destroy_actctx = mk::make_scope_exit([&](){ destroy_actctx(actctx_state); });
	for(std::uint16_t i = 0; i != n; ++i)
	{
		bool const step = step_3(fi, i, to);
		WARN_M_R(step, L"Failed to step_3.", false);
	}
	return true;
}

bool step_3(file_info const& fi, std::uint16_t const i, tmp_type& to)
{
	file_info& sub_fi = fi.m_fis[i];
	dependency_locator& dl = to.m_dl;
	dl.m_dependency = &fi.m_import_table.m_dll_names[i];
	bool const located = locate_dependency(dl);
	if(located)
	{
		std::wstring const& result = dl.m_result;
		wstring_handle const normalized = file_name_provider::get_correct_file_name(result.c_str(), static_cast<int>(result.size()), to.m_mm->m_wstrs, to.m_mm->m_alc);
		to.m_queue.push_back({normalized, &sub_fi});
		return true;
	}
	else
	{
		return true;
	}
}
