#pragma once


#include "pe.h"
#include "pe_getters.h"

#include <cstdint>


std::uint8_t pe_get_export_icon_id(pe_export_table_info const& eti, std::uint16_t const* const matched_imports, std::uint16_t const exp_idx);
bool pe_get_export_type(pe_export_table_info const& eti, std::uint16_t const exp_idx);
std::uint16_t pe_get_export_ordinal(pe_export_table_info const& eti, std::uint16_t const exp_idx);
optional<std::uint16_t> pe_get_export_hint(pe_export_table_info const& eti, std::uint16_t const exp_idx);
string_handle pe_get_export_name(pe_export_table_info const& eti, std::uint16_t const exp_idx);
string_handle pe_get_export_name_undecorated(pe_export_table_info const& eti, std::uint16_t const exp_idx);
pe_rva_or_forwarder pe_get_export_entry_point(pe_export_table_info const& eti, std::uint16_t const exp_idx);
