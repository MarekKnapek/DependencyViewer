#include "processor_impl.h"

#include "search.h"

#include "../nogui/array_bool.h"
#include "../nogui/file_name_provider.h"
#include "../nogui/manifest_parser.h"
#include "../nogui/memory_mapped_file.h"
#include "../nogui/pe2.h"
#include "../nogui/unicode.h"
#include "../nogui/unique_strings.h"
#include "../nogui/utils.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <queue>
#include <unordered_map>


#define s_very_big_int (2'147'483'647)


main_type process_impl(std::vector<std::wstring> const& file_paths)
{
	main_type mo;
	manifest_parser mp(mo.m_mm);
	processor_impl prcsr;
	prcsr.m_mo = &mo;
	prcsr.m_manifest_parser = &mp;

	file_info& fi = mo.m_fi;
	fi.m_orig_instance = nullptr;
	fi.m_file_path = nullptr;
	assert(file_paths.size() <= 0xffff);
	std::uint16_t const n = static_cast<std::uint16_t>(file_paths.size());
	my_vector_resize(fi.m_sub_file_infos, mo.m_mm.m_alc, n);
	for(int i = 0; i != n; ++i)
	{
		std::wstring p = std::filesystem::path(file_paths[i]).replace_filename(L"xxx.xxx");
		int const len = static_cast<int>(file_paths[i].size());
		fi.m_sub_file_infos[i].m_file_path = mo.m_mm.m_wstrs.add_string(p.c_str(), static_cast<int>(p.size()), mo.m_mm.m_alc);
		my_vector_resize(fi.m_sub_file_infos[i].m_sub_file_infos, mo.m_mm.m_alc, 1);
		wchar_t const* const name = find_file_name(file_paths[i].c_str(), len);
		int const name_len = static_cast<int>(file_paths[i].c_str() + len - name);
		assert(is_ascii(name, name_len));
		mo.m_tmpn.resize(name_len);
		std::transform(name, name + name_len, begin(mo.m_tmpn), [](wchar_t const& e) -> char { return static_cast<char>(e); });
		string const** const dll_names = mo.m_mm.m_alc.allocate_objects<string const*>(1);
		dll_names[0] = mo.m_mm.m_strs.add_string(mo.m_tmpn.c_str(), name_len, mo.m_mm.m_alc);
		fi.m_sub_file_infos[i].m_import_table.m_dll_count = 1;
		fi.m_sub_file_infos[i].m_import_table.m_non_delay_dll_count = 0;
		fi.m_sub_file_infos[i].m_import_table.m_dll_names = dll_names;
		static constexpr const std::uint16_t s_zero_imports = 0;
		fi.m_sub_file_infos[i].m_import_table.m_import_counts = &s_zero_imports;
		prcsr.m_queue.push(&fi.m_sub_file_infos[i]);
	}
	process_r(prcsr);
	return mo;
}

void process_r(processor_impl& prcsr)
{
	while(!prcsr.m_queue.empty())
	{
		file_info& fi = *prcsr.m_queue.front();
		prcsr.m_queue.pop();
		std::uint16_t const n = fi.m_import_table.m_dll_count;
		for(std::uint16_t i = 0; i != n; ++i)
		{
			file_info& sub_fi = fi.m_sub_file_infos[i];
			string const* const& sub_name = fi.m_import_table.m_dll_names[i];
			auto const it = prcsr.m_map.find(sub_name);
			if(it != prcsr.m_map.cend())
			{
				sub_fi.m_orig_instance = it->second.m_fi;
				prcsr.m_curr_enpt = &it->second.m_enpt;
			}
			else
			{
				auto& e = (prcsr.m_map[sub_name] = {&sub_fi, {}});
				sub_fi.m_orig_instance = nullptr;
				prcsr.m_curr_enpt = &e.m_enpt;
				process_e(prcsr, fi, sub_fi, sub_name);
				my_vector_resize(sub_fi.m_sub_file_infos, prcsr.m_mo->m_mm.m_alc, sub_fi.m_import_table.m_dll_count);
				prcsr.m_queue.push(&sub_fi);
			}
			pair_imports_with_exports(prcsr, fi, sub_fi);
			pair_exports_with_imports(prcsr, fi, sub_fi);
		}
	}
}

void process_e(processor_impl& prcsr, file_info& fi, file_info& sub_fi, string const* const& dll_name)
{
	searcher sch;
	sch.m_mo = prcsr.m_mo;
	sch.m_main_file_path = fi.m_file_path;
	search(sch, dll_name);
	if(sch.m_mo->m_tmpw.empty())
	{
		sub_fi.m_file_path = get_not_found_string();
		return;
	}
	wstring const* const wstr = file_name_provider::get_correct_file_name(sch.m_mo->m_tmpw.c_str(), static_cast<int>(sch.m_mo->m_tmpw.size()), prcsr.m_mo->m_mm.m_wstrs, prcsr.m_mo->m_mm.m_alc);
	if(wstr)
	{
		sub_fi.m_file_path = wstr;
	}
	else
	{
		sub_fi.m_file_path = prcsr.m_mo->m_mm.m_wstrs.add_string(sch.m_mo->m_tmpw.c_str(), static_cast<int>(sch.m_mo->m_tmpw.size()), prcsr.m_mo->m_mm.m_alc);
	}

	memory_mapped_file const mmf = memory_mapped_file(sub_fi.m_file_path->m_str);
	pe_header_info const hi = pe_process_header(mmf.begin(), mmf.size());
	sub_fi.m_is_32_bit = hi.m_is_pe32;
	pe_tables tables;
	tables.m_tmp_alc = &prcsr.m_entp_alloc;
	tables.m_iti_out = &sub_fi.m_import_table;
	tables.m_eti_out = &sub_fi.m_export_table;
	tables.m_enpt_count_out = &prcsr.m_curr_enpt->m_count;
	tables.m_enpt_out = &prcsr.m_curr_enpt->m_enpt;
	bool const tables_processed = pe_process_all(mmf.begin(), mmf.size(), prcsr.m_mo->m_mm, &tables);
	if(!tables_processed)
	{
		wchar_t const err[] = L"Failed to process file.";
		throw static_cast<wchar_t const*>(err);
	}
	sub_fi.m_resources_table = pe_process_resource_table(mmf.begin(), mmf.size(), hi, prcsr.m_mo->m_mm);
	sub_fi.m_manifest_data = process_manifest(prcsr, sub_fi);
}

manifest_data process_manifest(processor_impl& prcsr, file_info const& fi)
{
	auto const manifest = find_manifest(fi);
	if(!manifest.first)
	{
		return {};
	}
	return prcsr.m_manifest_parser->parse(manifest.first, manifest.second);
}

std::pair<std::byte const*, int> find_manifest(file_info const& fi)
{
	for(auto&& e : fi.m_resources_table.m_resources)
	{
		if(!e.m_type.m_is_string)
		{
			static_assert(sizeof(RT_MANIFEST) == sizeof(void*), "");
			if(e.m_type.m_id == static_cast<std::uint16_t>(reinterpret_cast<std::uintptr_t>(RT_MANIFEST)))
			{
				if(!e.m_name.m_is_string)
				{
					if(e.m_name.m_id == 1 || e.m_name.m_id == 2)
					{
						// Will get first manifest ID 1 or ID 2.
						// Sadly, we don't distinguish between DLLs and EXEs.
						// Also, we don't care about language, but we probably should.
						if(e.m_size < s_very_big_int)
						{
							return {e.m_data, e.m_size};
						}
					}
				}
			}
		}
	}
	return {};
}

void pair_imports_with_exports(processor_impl& prcsr, file_info& fi, file_info& sub_fi)
{
	file_info& sub_fi_proper = sub_fi.m_orig_instance ? *sub_fi.m_orig_instance : sub_fi;
	pe_export_table_info& exp = sub_fi_proper.m_export_table;
	std::uint16_t const dll_idx = static_cast<std::uint16_t>(&sub_fi - fi.m_sub_file_infos.data());
	std::uint16_t const n = fi.m_import_table.m_import_counts[dll_idx];
	for(int i = 0; i != n; ++i)
	{
		std::uint16_t& matched_export = fi.m_import_table.m_matched_exports[dll_idx][i];
		bool const is_ordinal = array_bool_tst(fi.m_import_table.m_are_ordinals[dll_idx], i);
		if(is_ordinal)
		{
			std::uint16_t const ordinal = fi.m_import_table.m_ordinals_or_hints[dll_idx][i];
			std::uint16_t const ordinal_as_idx = ordinal - exp.m_ordinal_base;
			if(ordinal_as_idx < exp.m_count && exp.m_ordinals[ordinal_as_idx] == ordinal)
			{
				matched_export = ordinal_as_idx;
			}
			else
			{
				auto const ordinals_end = exp.m_ordinals + exp.m_count;
				auto const it = std::lower_bound(exp.m_ordinals, ordinals_end, ordinal, [](auto const& e, auto const& v){ return e < v; });
				if(it != ordinals_end && *it == ordinal)
				{
					matched_export = static_cast<std::uint16_t>(it - exp.m_ordinals);
				}
				else
				{
					matched_export = 0xffff;
				}
			}
		}
		else
		{
			std::uint16_t const hint = fi.m_import_table.m_ordinals_or_hints[dll_idx][i];
			string const* const name = fi.m_import_table.m_names[dll_idx][i];
			auto const& enpt = *prcsr.m_curr_enpt;
			if(hint < enpt.m_count && string_equal{}(exp.m_names[enpt.m_enpt[hint]], name))
			{
				matched_export = enpt.m_enpt[hint];
			}
			else
			{
				auto const enpt_end = enpt.m_enpt + enpt.m_count;
				auto const it = std::lower_bound(enpt.m_enpt, enpt_end, name, [&](auto const& e, auto const& v){ return string_less{}(exp.m_names[e], v); });
				if(it != enpt_end && string_equal{}(exp.m_names[*it], name))
				{
					matched_export = *it;
				}
				else
				{
					matched_export = 0xffff;
				}
			}
		}
		#define ordinal_macro (fi.m_import_table.m_ordinals_or_hints[dll_idx][i])
		#define name_macro (fi.m_import_table.m_names[dll_idx][i])
		assert(matched_export == 0xffff || (is_ordinal ? (ordinal_macro == exp.m_ordinals[matched_export]) : (string_equal{}(name_macro, exp.m_names[matched_export]))));
		#undef name_macro
		#undef ordinal_macro
	}
}

void pair_exports_with_imports(processor_impl& prcsr, file_info& fi, file_info& sub_fi)
{
	file_info& sub_fi_proper = sub_fi.m_orig_instance ? *sub_fi.m_orig_instance : sub_fi;
	pe_export_table_info& exp = sub_fi_proper.m_export_table;
	sub_fi.m_matched_imports = prcsr.m_mo->m_mm.m_alc.allocate_objects<std::uint16_t>(exp.m_count);
	std::fill(sub_fi.m_matched_imports, sub_fi.m_matched_imports + exp.m_count, static_cast<std::uint16_t>(0xffff));
	int const dll_idx = static_cast<int>(&sub_fi - fi.m_sub_file_infos.data());
	std::uint16_t const n_imports = fi.m_import_table.m_import_counts[dll_idx];
	for(std::uint16_t i = 0; i != n_imports; ++i)
	{
		std::uint16_t const matched_export = fi.m_import_table.m_matched_exports[dll_idx][i];
		if(matched_export == 0xffff)
		{
			continue;
		}
		sub_fi.m_matched_imports[matched_export] = i;
		if(!array_bool_tst(exp.m_are_used, matched_export))
		{
			array_bool_set(exp.m_are_used, matched_export);
		}
	}
}
