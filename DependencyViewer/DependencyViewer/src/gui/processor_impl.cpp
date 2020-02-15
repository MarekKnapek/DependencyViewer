#include "processor_impl.h"

#include "file_info_getters.h"
#include "import_export_matcher.h"
#include "processor.h"
#include "tree_algos.h"

#include "../nogui/act_ctx.h"
#include "../nogui/assert_my.h"
#include "../nogui/cassert_my.h"
#include "../nogui/dependency_locator.h"
#include "../nogui/file_name_provider.h"
#include "../nogui/memory_mapped_file.h"
#include "../nogui/my_actctx.h"
#include "../nogui/pe2.h"
#include "../nogui/scope_exit.h"

#include <algorithm>
#include <cstdint>
#include <iterator>


static constexpr wchar_t const s_dummy_textw_r[] = L"";
static constexpr wstring const s_dummy_textw_s = {s_dummy_textw_r, static_cast<int>(std::size(s_dummy_textw_r)) - 1};
static constexpr wstring_handle const s_dummy_textw_h = {&s_dummy_textw_s};
static constexpr char const s_dummy_texta_r[] = "";
static constexpr string const s_dummy_texta_s = {s_dummy_texta_r, static_cast<int>(std::size(s_dummy_texta_r)) - 1};
static constexpr string_handle const s_dummy_texta_h = {&s_dummy_texta_s};


bool process_impl(std::vector<std::wstring> const& file_paths, main_type& mo)
{
	my_actctx::deactivate();
	auto const activate_my_actctx = mk::make_scope_exit([](){ my_actctx::activate(); });
	WARN_M_R(file_paths.size() < 0xFFFF, L"Too many files to process.", false);
	file_info* const fi = mo.m_mm.m_alc.allocate_objects<file_info>(1);
	init(fi);
	mo.m_fi = fi;
	std::uint16_t const n = static_cast<std::uint16_t>(file_paths.size());
	file_info* const fis = mo.m_mm.m_alc.allocate_objects<file_info>(n);
	init(fis, n);
	string_handle* const dll_names = mo.m_mm.m_alc.allocate_objects<string_handle>(n);
	std::fill(dll_names, dll_names + n, s_dummy_texta_h);
	std::uint16_t* const import_counts = mo.m_mm.m_alc.allocate_objects<std::uint16_t>(n);
	std::fill(import_counts, import_counts + n, std::uint16_t{0});
	fi->m_fis = fis;
	fi->m_file_path = s_dummy_textw_h;
	fi->m_import_table.m_normal_dll_count = n;
	fi->m_import_table.m_delay_dll_count = 0;
	fi->m_import_table.m_dll_names = dll_names;
	fi->m_import_table.m_import_counts = import_counts;
	tmp_type to;
	to.m_mo = &mo;
	to.m_mm = &mo.m_mm;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& sub_fi = fi->m_fis[i];
		int const path_len = static_cast<int>(file_paths[i].size());
		wchar_t const* const cstr = file_paths[i].c_str();
		wstring_handle const normalized = file_name_provider::get_correct_file_name(cstr, path_len, to.m_mm->m_wstrs, to.m_mm->m_alc);
		sub_fi.m_file_path = normalized;
		dependency_locator& dl = to.m_dl;
		dl.m_main_path = normalized;
		assert(to.m_queue.empty());
		to.m_queue.push_back(&sub_fi);
		bool const step = step_1(to);
		WARN_M_R(step, L"Failed to step_1.", false);
	}
	pair_all(*fi, to);
	make_doubly_linked_list(*fi);
	mo.m_modules_list = make_modules_list(to);
	return true;
}


void make_doubly_linked_list(file_info& fi)
{
	static constexpr auto const make_list = [](file_info& fi, [[maybe_unused]] void* const data)
	{
		file_info* const orig = fi.m_orig_instance;
		if(!orig)
		{
			return;
		}
		fi.m_next_instance = orig;
		fi.m_prev_instance = orig->m_prev_instance ? orig->m_prev_instance : orig;
		(orig->m_prev_instance ? orig->m_prev_instance : orig)->m_next_instance = &fi;
		orig->m_prev_instance = &fi;
	};
	depth_first_visit(fi, make_list, nullptr);
}

modules_list_t make_modules_list(tmp_type const& to)
{
	static constexpr auto const make_non_found_modules = [](tmp_type const& to)
	{
		struct case_insensitive_file_info_file_name_hash_t
		{
			std::size_t operator()(file_info const* const& fi) const
			{
				assert(fi);
				auto const& dll_name = get_dll_name_no_path(fi);
				assert(dll_name);
				auto const ret = string_handle_case_insensitive_hash{}(dll_name);
				return ret;
			}
		};
		struct case_insensitive_file_info_file_name_equals_t
		{
			bool operator()(file_info const* const& a, file_info const* const& b) const
			{
				assert(a);
				assert(b);
				auto const& dll_name_a = get_dll_name_no_path(a);
				auto const& dll_name_b = get_dll_name_no_path(b);
				assert(dll_name_a);
				assert(dll_name_b);
				auto const ret = string_handle_case_insensitive_equal{}(dll_name_a, dll_name_b);
				return ret;
			}
		};
		typedef std::unordered_set<file_info*, case_insensitive_file_info_file_name_hash_t, case_insensitive_file_info_file_name_equals_t> not_found_fis_t;

		static constexpr auto const process_not_found_fi = [](file_info& fi, void* const data)
		{
			if(!fi.m_orig_instance && !fi.m_file_path)
			{
				assert(data);
				auto& not_found_fis = *static_cast<not_found_fis_t*>(data);
				not_found_fis.insert(&fi);
			}
		};

		not_found_fis_t not_found_fis;
		depth_first_visit(*to.m_mo->m_fi, process_not_found_fi, &not_found_fis);
		return not_found_fis;
	};

	auto const not_found_fis = make_non_found_modules(to);
	int const n1 = static_cast<int>(not_found_fis.size());
	int const n2 = static_cast<int>(to.m_map.size());
	assert(n1 <= 0xFFFF && n2 <= 0xFFFF && n1 + n2 <= 0xFFFF);
	std::uint16_t const n = static_cast<std::uint16_t>(n1 + n2);
	file_info** const modules_list = to.m_mm->m_alc.allocate_objects<file_info*>(n);
	std::copy(not_found_fis.begin(), not_found_fis.end(), modules_list);
	std::transform(to.m_map.begin(), to.m_map.end(), modules_list + n1, [](auto const& e){ return e->m_instance; });
	std::sort(modules_list, modules_list + n, compare_fi_by_path_or_name);

	modules_list_t ret;
	ret.m_list = modules_list;
	ret.m_count = n;
	return ret;
}


bool step_1(tmp_type& to)
{
	while(!to.m_queue.empty())
	{
		file_info* const fi = to.m_queue.front();
		assert(fi != nullptr);
		to.m_queue.pop_front();
		bool const step = step_2(*fi, to);
		WARN_M_R(step, L"Failed to step_2.", false);
	}
	return true;
}

bool step_2(file_info& fi, tmp_type& to)
{
	fat_type tmp;
	tmp.m_instance = &fi;
	auto const it = to.m_map.find(&tmp);
	if(it != to.m_map.end())
	{
		assert((*it)->m_instance);
		file_info* const orig = (*it)->m_instance;
		fi.m_orig_instance = orig;
		fi.m_file_path = wstring_handle{};
		return true;
	}
	std::uint16_t const* enpt;
	std::uint16_t enpt_count;
	pe_tables tables;
	tables.m_tmp_alc = &to.m_tmp_alc;
	tables.m_iti_out = &fi.m_import_table;
	tables.m_eti_out = &fi.m_export_table;
	tables.m_enpt_count_out = &enpt_count;
	tables.m_enpt_out = &enpt;
	{
		memory_mapped_file const mmf = memory_mapped_file(fi.m_file_path.m_string->m_str);
		WARN_M_R(mmf.begin() != nullptr, L"Failed to memory_mapped_file.", false);
		bool const tables_processed = pe_process_all(mmf.begin(), mmf.size(), *to.m_mm, &tables);
		WARN_M_R(tables_processed, L"Failed to pe_process_all.", false);
	}
	fi.m_is_32_bit = tables.m_is_32_bit;
	fat_type* const fo = to.m_tmp_alc.allocate_objects<fat_type>(1);
	fo->m_instance = &fi;
	fo->m_enpt.m_table = enpt;
	fo->m_enpt.m_count = enpt_count;
	auto const itb = to.m_map.insert(fo);
	assert(itb.second);
	std::uint16_t const n = fi.m_import_table.m_normal_dll_count + fi.m_import_table.m_delay_dll_count;
	file_info* const fis = to.m_mm->m_alc.allocate_objects<file_info>(n);
	init(fis, n);
	std::for_each(fis, fis + n, [&](file_info& sub_fi){ sub_fi.m_parent = &fi; });
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
		sub_fi.m_file_path = normalized;
		to.m_queue.push_back(&sub_fi);
		return true;
	}
	else
	{
		return true;
	}
}
