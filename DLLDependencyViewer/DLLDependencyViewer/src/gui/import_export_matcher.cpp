#include "import_export_matcher.h"

#include "tree_algos.h"

#include "../nogui/array_bool.h"

#include <algorithm>
#include <cassert>


void pair_all(file_info& fi, tmp_type& to)
{
	static constexpr auto const pair_fn = [](file_info& fi, void* const data)
	{
		assert(data);
		tmp_type& to = *static_cast<tmp_type*>(data);
		std::uint16_t const n = fi.m_import_table.m_normal_dll_count + fi.m_import_table.m_delay_dll_count;
		for(std::uint16_t i = 0; i != n; ++i)
		{
			file_info& sub_fi = fi.m_fis[i];
			pair_imports_with_exports(fi, sub_fi, to);
			pair_exports_with_imports(fi, sub_fi, to);
		}
	};
	depth_first_visit(fi, pair_fn, &to);
}


void pair_imports_with_exports(file_info& fi, file_info& sub_fi, tmp_type& to)
{
	file_info& sub_fi_proper = sub_fi.m_orig_instance ? *sub_fi.m_orig_instance : sub_fi;
	if(sub_fi_proper.m_file_path.m_string == nullptr)
	{
		return;
	}
	pe_export_table_info& exp = sub_fi_proper.m_export_table;
	auto const dll_idx_ = &sub_fi - fi.m_fis;
	assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
	std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
	std::uint16_t const n = fi.m_import_table.m_import_counts[dll_idx];
	for(int i = 0; i != n; ++i)
	{
		std::uint16_t& matched_export = fi.m_import_table.m_matched_exports[dll_idx][i];
		bool const is_ordinal = array_bool_tst(fi.m_import_table.m_are_ordinals[dll_idx], i);
		if(is_ordinal)
		{
			std::uint16_t const& ordinal = fi.m_import_table.m_ordinals_or_hints[dll_idx][i];
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
				}
			}
		}
		else
		{
			std::uint16_t const& hint = fi.m_import_table.m_ordinals_or_hints[dll_idx][i];
			string_handle const& name = fi.m_import_table.m_names[dll_idx][i];
			fat_type tmp;
			tmp.m_orig_instance = sub_fi.m_orig_instance ? sub_fi.m_orig_instance : &sub_fi;
			auto const it_2 = to.m_map.find(&tmp);
			assert(it_2 != to.m_map.end());
			auto const& enpt = (*it_2)->m_enpt;
			if(hint < enpt.m_count && exp.m_names[enpt.m_table[hint]] == name)
			{
				matched_export = enpt.m_table[hint];
			}
			else
			{
				auto const enpt_end = enpt.m_table + enpt.m_count;
				auto const it = std::lower_bound(enpt.m_table, enpt_end, name, [&](auto const& e, auto const& v) -> bool { return exp.m_names[e] < v; });
				if(it != enpt_end && exp.m_names[*it] == name)
				{
					matched_export = *it;
				}
				else
				{
				}
			}
		}
		#define ordinal_macro (fi.m_import_table.m_ordinals_or_hints[dll_idx][i])
		#define name_macro (fi.m_import_table.m_names[dll_idx][i])
		assert(matched_export == 0xFFFF || (is_ordinal ? (ordinal_macro == exp.m_ordinals[matched_export]) : (name_macro == exp.m_names[matched_export])));
		#undef name_macro
		#undef ordinal_macro
	}
}

void pair_exports_with_imports(file_info& fi, file_info& sub_fi, tmp_type& to)
{
	file_info& sub_fi_proper = sub_fi.m_orig_instance ? *sub_fi.m_orig_instance : sub_fi;
	if(sub_fi_proper.m_file_path.m_string == nullptr)
	{
		return;
	}
	pe_export_table_info& exp = sub_fi_proper.m_export_table;
	sub_fi.m_matched_imports = to.m_mm->m_alc.allocate_objects<std::uint16_t>(exp.m_count);
	std::fill(sub_fi.m_matched_imports, sub_fi.m_matched_imports + exp.m_count, static_cast<std::uint16_t>(0xFFFF));
	auto const dll_idx_ = &sub_fi - fi.m_fis;
	assert(dll_idx_ >= 0 && dll_idx_ <= 0xFFFF);
	std::uint16_t const dll_idx = static_cast<std::uint16_t>(dll_idx_);
	std::uint16_t const& n_imports = fi.m_import_table.m_import_counts[dll_idx];
	for(std::uint16_t i = 0; i != n_imports; ++i)
	{
		std::uint16_t const& matched_export = fi.m_import_table.m_matched_exports[dll_idx][i];
		if(matched_export == 0xFFFF)
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
