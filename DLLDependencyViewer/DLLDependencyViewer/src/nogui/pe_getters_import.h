#pragma once


#include "pe.h"
#include "pe_getters.h"

#include <cstdint>


std::uint8_t pe_get_import_icon_id(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
bool pe_get_import_is_ordinal(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
optional<std::uint16_t> pe_get_import_ordinal(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
optional<std::uint16_t> pe_get_import_hint(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
string_handle pe_get_import_name(pe_import_table_info const& iti, pe_export_table_info const& eti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
string_handle pe_get_import_name_undecorated(pe_import_table_info const& iti, pe_export_table_info const& eti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
