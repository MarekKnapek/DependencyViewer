#include "pe_getters_export.h"

#include "array_bool.h"
#include "cassert_my.h"

#include "../res/resources.h"


std::uint8_t pe_get_export_icon_id(pe_export_table_info const& eti, std::uint16_t const* const matched_imports, std::uint16_t const exp_idx)
{
	bool const is_used = array_bool_tst(eti.m_are_used, exp_idx);
	bool const is_matched = (matched_imports != nullptr) ? (matched_imports[exp_idx] != 0xFFFF) : false;
	bool const is_rva = array_bool_tst(eti.m_are_rvas, exp_idx);
	bool const has_name = eti.m_hints[exp_idx] != 0xFFFF;
	static constexpr std::int8_t const s_export_images[2][2][2][2] =
	{
		{
			{
				{
					s_res_icon_export0000,
					s_res_icon_export0001,
				},
				{
					s_res_icon_export0010,
					s_res_icon_export0011,
				},
			},
			{
				{
					-1,
					-1,
				},
				{
					-1,
					-1,
				},
			},
		},
		{
			{
				{
					s_res_icon_export1000,
					s_res_icon_export1001,
				},
				{
					s_res_icon_export1010,
					s_res_icon_export1011,
				},
			},
			{
				{
					s_res_icon_export1100,
					s_res_icon_export1101,
				},
				{
					s_res_icon_export1110,
					s_res_icon_export1111,
				},
			},
		},
	};
	std::int8_t const& img = s_export_images[is_used ? 1 : 0][is_matched ? 1 : 0][is_rva ? 1 : 0][has_name ? 1 : 0];
	assert(img != -1);
	return img;
}

bool pe_get_export_type(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	bool const is_rva = array_bool_tst(eti.m_are_rvas, exp_idx);
	return is_rva;
}

std::uint16_t pe_get_export_ordinal(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	std::uint16_t const ordinal = eti.m_ordinals[exp_idx];
	return ordinal;
}

optional<std::uint16_t> pe_get_export_hint(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	bool const has_name = eti.m_hints[exp_idx] != 0xFFFF;
	if(has_name)
	{
		std::uint16_t const hint = eti.m_hints[exp_idx];
		return {hint, true};
	}
	else
	{
		return {{}, false};
	}
}

string_handle pe_get_export_name(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	bool const has_name = eti.m_hints[exp_idx] != 0xFFFF;
	if(has_name)
	{
		string_handle const& name = eti.m_names[exp_idx];
		return name;
	}
	else
	{
		string_handle const& debug_name = eti.m_names[exp_idx];
		if(!debug_name.m_string)
		{
			bool const is_rva = array_bool_tst(eti.m_are_rvas, exp_idx);
			if(is_rva)
			{
				return get_export_name_processing();
			}
			else
			{
				return string_handle{nullptr};
			}
		}
		else if(debug_name.m_string == static_cast<string const*>(nullptr) + 1)
		{
			return string_handle{nullptr};
		}
		else
		{
			return debug_name;
		}
	}
}

string_handle pe_get_export_name_undecorated(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	bool const has_name = eti.m_hints[exp_idx] != 0xFFFF;
	if(has_name)
	{
		string_handle const& name = eti.m_names[exp_idx];
		bool const need_undecorating = cbegin(name)[0] == '?';
		if(need_undecorating)
		{
			string_handle const& undecorated_name = eti.m_undecorated_names[exp_idx];
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
	else
	{
		string_handle const& debug_name = eti.m_names[exp_idx];
		if(!debug_name.m_string)
		{
			bool const is_rva = array_bool_tst(eti.m_are_rvas, exp_idx);
			if(is_rva)
			{
				return get_export_name_processing();
			}
			else
			{
				return string_handle{nullptr};
			}
		}
		else if(debug_name.m_string == static_cast<string const*>(nullptr) + 1)
		{
			return string_handle{nullptr};
		}
		else
		{
			bool const need_undecorating = cbegin(debug_name)[0] == '?';
			if(need_undecorating)
			{
				string_handle const& undecorated_name = eti.m_undecorated_names[exp_idx];
				if(!undecorated_name.m_string)
				{
					return get_name_undecorating();
				}
				else if(undecorated_name.m_string == static_cast<string const*>(nullptr) + 1)
				{
					return debug_name;
				}
				else
				{
					return undecorated_name;
				}
			}
			else
			{
				return debug_name;
			}
		}
	}
}

pe_rva_or_forwarder pe_get_export_entry_point(pe_export_table_info const& eti, std::uint16_t const exp_idx)
{
	pe_rva_or_forwarder const& entry_point = eti.m_rvas_or_forwarders[exp_idx];
	return entry_point;
}
