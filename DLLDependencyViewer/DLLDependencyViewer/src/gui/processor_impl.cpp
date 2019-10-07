#include "processor_impl.h"

#include "search.h"

#include "../nogui/file_name.h"
#include "../nogui/manifest_parser.h"
#include "../nogui/memory_mapped_file.h"
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
	file_name fn;
	manifest_parser mp(mo.m_mm);
	processor_impl prcsr;
	prcsr.m_mo = &mo;
	prcsr.m_file_name = &fn;
	prcsr.m_manifest_parser = &mp;

	file_info& fi = mo.m_fi;
	fi.m_orig_instance = nullptr;
	fi.m_file_path = nullptr;
	int const n = static_cast<int>(file_paths.size());
	my_vector_resize(fi.m_sub_file_infos, mo.m_mm.m_alc, n);
	my_vector_resize(fi.m_import_table.m_dlls, mo.m_mm.m_alc, n);
	for(int i = 0; i != n; ++i)
	{
		int const len = static_cast<int>(file_paths[i].size());
		fi.m_sub_file_infos[i].m_file_path = mo.m_mm.m_wstrs.add_string(file_paths[i].c_str(), len, mo.m_mm.m_alc);
		my_vector_resize(fi.m_sub_file_infos[i].m_sub_file_infos, mo.m_mm.m_alc, 1);
		my_vector_resize(fi.m_sub_file_infos[i].m_import_table.m_dlls, mo.m_mm.m_alc, 1);
		wchar_t const* const name = find_file_name(file_paths[i].c_str(), len);
		int const name_len = static_cast<int>(file_paths[i].c_str() + len - name);
		assert(is_ascii(name, name_len));
		mo.m_tmpn.resize(name_len);
		std::transform(name, name + name_len, begin(mo.m_tmpn), [](wchar_t const& e) -> char { return static_cast<char>(e); });
		fi.m_sub_file_infos[i].m_import_table.m_dlls[0].m_dll_name = mo.m_mm.m_strs.add_string(mo.m_tmpn.c_str(), name_len, mo.m_mm.m_alc);
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
		int const n = static_cast<int>(fi.m_import_table.m_dlls.size());
		for(int i = 0; i != n; ++i)
		{
			file_info& sub_fi = fi.m_sub_file_infos[i];
			string const* const& sub_name = fi.m_import_table.m_dlls[i].m_dll_name;
			auto const it = prcsr.m_map.find(sub_name);
			if(it != prcsr.m_map.cend())
			{
				sub_fi.m_orig_instance = it->second;
			}
			else
			{
				process_e(prcsr, fi, sub_fi, sub_name);
				prcsr.m_map[sub_name] = &sub_fi;
				my_vector_resize(sub_fi.m_sub_file_infos, prcsr.m_mo->m_mm.m_alc, static_cast<int>(sub_fi.m_import_table.m_dlls.size()));
				prcsr.m_queue.push(&sub_fi);
			}
		}
	}
	pair_imports_with_exports(prcsr);
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
	wstring const* const wstr = prcsr.m_file_name->get_correct_file_name(sch.m_mo->m_tmpw.c_str(), static_cast<int>(sch.m_mo->m_tmpw.size()), prcsr.m_mo->m_mm.m_wstrs, prcsr.m_mo->m_mm.m_alc);
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
	sub_fi.m_import_table = pe_process_import_table(mmf.begin(), mmf.size(), hi, prcsr.m_mo->m_mm);
	sub_fi.m_export_table = pe_process_export_table(mmf.begin(), mmf.size(), hi, prcsr.m_mo->m_mm);
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

std::pair<char const*, int> find_manifest(file_info const& fi)
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

void pair_imports_with_exports(processor_impl& prcsr)
{
	assert(prcsr.m_mo->m_fi.m_sub_file_infos.size() >= 1);
	pair_imports_with_exports_r(prcsr.m_mo->m_fi, prcsr);
}

void pair_imports_with_exports_r(file_info& fi, processor_impl& prcsr)
{
	for(auto& sub_fi : fi.m_sub_file_infos)
	{
		pair_imports_with_exports_e(sub_fi);
		pair_exports_with_imports_e(fi, sub_fi, prcsr);
		pair_imports_with_exports_r(sub_fi, prcsr);
	}
}

void pair_imports_with_exports_e(file_info& fi)
{
	assert(fi.m_import_table.m_dlls.size() == fi.m_sub_file_infos.size());
	int const n = static_cast<int>(fi.m_import_table.m_dlls.size());
	for(int i = 0; i != n; ++i)
	{
		pe_import_dll_with_entries& dll = fi.m_import_table.m_dlls[i];
		pe_export_table_info const& exp = fi.m_sub_file_infos[i].m_orig_instance ? fi.m_sub_file_infos[i].m_orig_instance->m_export_table : fi.m_sub_file_infos[i].m_export_table;
		auto const& eat = exp.m_export_address_table;
		int const m = static_cast<int>(dll.m_entries.size());
		for(int j = 0; j != m; ++j)
		{
			pe_import_entry& impe = dll.m_entries[j];
			if(impe.m_is_ordinal)
			{
				std::uint16_t const ordinal_as_idx = impe.m_ordinal_or_hint - exp.m_ordinal_base;
				if(ordinal_as_idx < eat.size() && eat[ordinal_as_idx].m_ordinal == impe.m_ordinal_or_hint)
				{
					impe.m_matched_export = ordinal_as_idx;
				}
				else
				{
					auto const it = std::lower_bound(eat.cbegin(), eat.cend(), impe.m_ordinal_or_hint, [](auto const& e, auto const& v){ return e.m_ordinal < v; });
					if(it != eat.cend() && it->m_ordinal == impe.m_ordinal_or_hint)
					{
						assert(eat.size() <= 0xffff);
						impe.m_matched_export = static_cast<std::uint16_t>(it - eat.cbegin());
					}
					else
					{
						impe.m_matched_export = 0xffff;
					}
				}
			}
			else
			{
				std::uint16_t const& hint = impe.m_ordinal_or_hint;
				string const* const& name = impe.m_name;
				auto const& enpt = exp.m_enpt;
				if(hint < enpt.size() && string_equal{}(eat[enpt[hint]].m_name, name))
				{
					impe.m_matched_export = enpt[hint];
				}
				else
				{
					auto const it = std::lower_bound(enpt.cbegin(), enpt.cend(), name, [&](auto const& e, auto const& v){ return string_less{}(eat[e].m_name, v); });
					if(it != enpt.cend() && string_equal{}(eat[*it].m_name, name))
					{
						impe.m_matched_export = *it;
					}
					else
					{
						impe.m_matched_export = 0xffff;
					}
				}
			}
			assert(impe.m_matched_export == 0xffff || (impe.m_is_ordinal ? (impe.m_ordinal_or_hint == eat[impe.m_matched_export].m_ordinal) : (string_equal{}(impe.m_name, eat[impe.m_matched_export].m_name))));
		}
	}
}

void pair_exports_with_imports_e(file_info& fi, file_info& sub_fi, processor_impl& prcsr)
{
	pe_export_table_info& exp = sub_fi.m_orig_instance ? sub_fi.m_orig_instance->m_export_table : sub_fi.m_export_table;
	int const n = static_cast<int>(exp.m_export_address_table.size());
	sub_fi.m_matched_imports = prcsr.m_mo->m_mm.m_alc.allocate_objects<std::uint16_t>(n);
	std::fill(sub_fi.m_matched_imports, sub_fi.m_matched_imports + n, static_cast<std::uint16_t>(0xffff));
	int const dll_idx = static_cast<int>(&sub_fi - fi.m_sub_file_infos.data());
	auto const& imports = fi.m_import_table.m_dlls[dll_idx].m_entries;
	std::uint16_t const n_imports = static_cast<std::uint16_t>(imports.size());
	for(std::uint16_t i = 0; i != n_imports; ++i)
	{
		std::uint16_t const& matched_export = imports[i].m_matched_export;
		if(matched_export == 0xffff)
		{
			continue;
		}
		sub_fi.m_matched_imports[matched_export] = i;
		exp.m_export_address_table[matched_export].m_is_used = true;
	}
}
