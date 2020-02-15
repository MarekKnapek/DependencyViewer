#include "pe_getters_import.h"

#include "array_bool.h"
#include "cassert_my.h"
#include "pe_getters_export.h"

#include "../res/resources.h"


std::uint8_t pe_get_import_icon_id(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	std::uint16_t const& matched_export = iti.m_matched_exports[dll_idx][imp_idx];
	bool const has_matched_export = matched_export != 0xFFFF;
	bool const is_ordinal = array_bool_tst(iti.m_are_ordinals[dll_idx], imp_idx);
	if(has_matched_export && is_ordinal)
	{
		return s_res_icon_import_found_o;
	}
	else if(has_matched_export && !is_ordinal)
	{
		return s_res_icon_import_found_c;
	}
	else if(!has_matched_export && is_ordinal)
	{
		return s_res_icon_import_not_found_o;
	}
	else if(!has_matched_export && !is_ordinal)
	{
		return s_res_icon_import_not_found_c;
	}
	assert(false);
	__assume(false);
}

bool pe_get_import_is_ordinal(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	bool const is_ordinal = array_bool_tst(iti.m_are_ordinals[dll_idx], imp_idx);
	return is_ordinal;
}

optional<std::uint16_t> pe_get_import_ordinal(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	bool const is_ordinal = array_bool_tst(iti.m_are_ordinals[dll_idx], imp_idx);
	if(is_ordinal)
	{
		std::uint16_t const& ordinal = iti.m_ordinals_or_hints[dll_idx][imp_idx];
		return {ordinal, true};
	}
	else
	{
		return {{}, false};
	}
}

optional<std::uint16_t> pe_get_import_hint(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	bool const is_ordinal = array_bool_tst(iti.m_are_ordinals[dll_idx], imp_idx);
	if(is_ordinal)
	{
		return {{}, false};
	}
	else
	{
		std::uint16_t const& hint = iti.m_ordinals_or_hints[dll_idx][imp_idx];
		return {hint, true};
	}
}

string_handle pe_get_import_name(pe_import_table_info const& iti, pe_export_table_info const& eti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	bool const is_ordinal = array_bool_tst(iti.m_are_ordinals[dll_idx], imp_idx);
	if(is_ordinal)
	{
		std::uint16_t const& matched_export = iti.m_matched_exports[dll_idx][imp_idx];
		bool const has_matched_export = matched_export != 0xFFFF;
		if(has_matched_export)
		{
			return pe_get_export_name(eti, matched_export);
		}
		else
		{
			return string_handle{nullptr};
		}
	}
	else
	{
		string_handle const& name = iti.m_names[dll_idx][imp_idx];
		return name;
	}
}

string_handle pe_get_import_name_undecorated(pe_import_table_info const& iti, pe_export_table_info const& eti, std::uint16_t const dll_idx, std::uint16_t const imp_idx)
{
	bool const is_ordinal = array_bool_tst(iti.m_are_ordinals[dll_idx], imp_idx);
	if(is_ordinal)
	{
		std::uint16_t const& matched_export = iti.m_matched_exports[dll_idx][imp_idx];
		bool const has_matched_export = matched_export != 0xFFFF;
		if(has_matched_export)
		{
			return pe_get_export_name_undecorated(eti, matched_export);
		}
		else
		{
			return string_handle{nullptr};
		}
	}
	else
	{
		string_handle const& name = iti.m_names[dll_idx][imp_idx];
		bool const need_undecorating = cbegin(name)[0] == '?';
		if(need_undecorating)
		{
			string_handle const& undecorated_name = iti.m_undecorated_names[dll_idx][imp_idx];
			if(!undecorated_name.m_string)
			{
				return get_name_undecorating();
			}
			else if(undecorated_name.m_string == static_cast<string const*>(nullptr) + 1)
			{
				return name;
			}
			else
			{
				return undecorated_name;
			}
		}
		else
		{
			return name;
		}
	}
}
