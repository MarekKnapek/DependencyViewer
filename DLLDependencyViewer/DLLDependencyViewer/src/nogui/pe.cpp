#include "pe.h"

#include "assert.h"
#include "memory_manager.h"
#include "unicode.h"

#include "pe/coff_full.h"
#include "pe/import_table.h"
#include "pe/mz.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>

#include "my_windows.h"


enum class data_directory_type
{
	export_table = 0,
	import_table,
	resource_table,
	exception_table,
	certificate_table,
	relocation_table,
	debug,
	architecture,
	global_ptr,
	tls_table,
	load_config_table,
	bound_import_table,
	import_address_table,
	delay_import_descriptor,
	clr_header,
	reserved
};

struct data_directory
{
	std::uint32_t m_rva;
	std::uint32_t m_size;
};
static_assert(sizeof(data_directory) == 8, "");
static_assert(sizeof(data_directory) == 0x8, "");

struct section_header
{
	std::array<std::uint8_t, 8> m_name;
	std::uint32_t m_virtual_size;
	std::uint32_t m_virtual_address;
	std::uint32_t m_raw_size;
	std::uint32_t m_raw_ptr;
	std::uint32_t m_relocations;
	std::uint32_t m_line_numbers;
	std::uint16_t m_relocation_count;
	std::uint16_t m_line_numbers_count;
	std::uint32_t m_characteristics;
};
static_assert(sizeof(section_header) == 40, "");
static_assert(sizeof(section_header) == 0x28, "");

struct export_directory
{
	std::uint32_t m_characteristics;
	std::uint32_t m_date_time;
	std::uint16_t m_major;
	std::uint16_t m_minor;
	std::uint32_t m_name_rva;
	std::uint32_t m_ordinal_base;
	std::uint32_t m_export_address_count;
	std::uint32_t m_names_count;
	std::uint32_t m_export_address_table_rva;
	std::uint32_t m_export_name_table_rva;
	std::uint32_t m_ordinal_table_rva;
};
static_assert(sizeof(export_directory) == 40, "");
static_assert(sizeof(export_directory) == 0x28, "");

struct resource_directory_table
{
	std::uint32_t m_characteristics;
	std::uint32_t m_time_date_stamp;
	std::uint16_t m_major_version;
	std::uint16_t m_minor_version;
	std::uint16_t m_number_of_name_entries;
	std::uint16_t m_number_of_id_entries;
};
static_assert(sizeof(resource_directory_table) == 16, "");
static_assert(sizeof(resource_directory_table) == 0x10, "");

struct resource_directory_entry
{
	union
	{
		std::uint32_t m_name_offset;
		std::uint16_t m_integer_id;
	};
	union
	{
		std::uint32_t m_data_entry_offset;
		std::uint32_t m_subdirectory_offset;
	};
};
static_assert(sizeof(resource_directory_entry) == 8, "");
static_assert(sizeof(resource_directory_entry) == 0x8, "");

struct resource_directory_string
{
	std::uint16_t m_length;
	char m_unicode_string[2];
};

struct resource_data_entry
{
	std::uint32_t m_data_rva;
	std::uint32_t m_size;
	std::uint32_t m_code_page;
	std::uint32_t m_reserved;
};
static_assert(sizeof(resource_data_entry) == 16, "");
static_assert(sizeof(resource_data_entry) == 0x10, "");

struct pe_resource_name_string_or_id_internal
{
	bool m_is_string;
	union string_or_id
	{
		struct string_with_length
		{
			std::uint16_t m_string_len;
			wchar_t const* m_string;
		};
		string_with_length m_string_with_length;
		std::uint16_t m_id;
	};
	string_or_id m_string_or_id;
};


static constexpr wchar_t const s_bad_format[] = L"Bad format.";


#define VERIFY(X) do{ if(!(X)){ assert((OutputDebugStringW(L"Error: " L ## #X L"\x0D\x0A"), true)); throw s_bad_format; } }while(false)
#define WARN(X) do{ if(!(X)){ assert((OutputDebugStringW(L"Warning: " L ## #X L"\x0D\x0A"), true)); } }while(false)


pe_resource_name_string_or_id_internal pe_resources_process_string(char const* const& file_data, std::uint32_t const& resource_directory_disk_offset, std::uint32_t const& resource_directory_size, bool const& is_string, resource_directory_entry const& dir_entry);
pe_resource_string_or_id convert_pe_string_to_string(pe_resource_name_string_or_id_internal const& str, memory_manager& mm);


pe_header_info pe_process_header(void const* const fd, int const file_size)
{
	char const* const file_data = static_cast<char const*>(fd);

	pe_dos_header const* dos_hdr;
	pe_e_parse_mz_header const mz_parsed = pe_parse_mz_header(file_data, file_size, dos_hdr);
	VERIFY(mz_parsed == pe_e_parse_mz_header::ok);

	pe_coff_full_32_64 const* coff_hdr;
	bool const coff_parsed = pe_parse_coff_full_32_64(file_data, file_size, coff_hdr);
	VERIFY(coff_parsed);
	bool const is_32 = coff_hdr->m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;

	pe_header_info ret;
	ret.m_file_data = file_data;
	ret.m_file_size = file_size;
	ret.m_pe_header_start = dos_hdr->m_pe_offset;
	ret.m_is_pe32 = is_32;
	ret.m_image_base = is_32 ? coff_hdr->m_32.m_windows.m_image_base : coff_hdr->m_64.m_windows.m_image_base;
	ret.m_data_directory_count = is_32 ? coff_hdr->m_32.m_windows.m_data_directory_count : coff_hdr->m_64.m_windows.m_data_directory_count;
	ret.m_data_directory_start = dos_hdr->m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64));
	ret.m_section_count = is_32 ? coff_hdr->m_32.m_coff.m_section_count : coff_hdr->m_64.m_coff.m_section_count;
	ret.m_section_headers_start = dos_hdr->m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)) + sizeof(pe_data_directory) * ret.m_data_directory_count;
	return ret;
}

pe_import_table_info pe_process_import_table(void const* const fd, int const fs, pe_header_info const& /*hi*/, memory_manager& mm)
{
	pe_import_table_info ret;
	auto const fn = [&]() -> bool
	{
		char const* const file_data = static_cast<char const*>(fd);
		int const file_size = static_cast<int>(fs);

		pe_import_directory_table idt;
		bool const import_descriptor_parsed = pe_parse_import_table(file_data, file_size, idt);
		WARN_M_R(import_descriptor_parsed, L"Failed to parse import descriptor", false);
		pe_delay_import_table dlit;
		bool const delay_descriptor_parsed = pe_parse_delay_import_table(file_data, file_size, dlit);
		WARN_M_R(delay_descriptor_parsed, L"Failed to parse delay import descriptor", false);
		ret.m_dlls.resize(idt.m_count + dlit.m_count);
		ret.m_nondelay_imports_count = idt.m_count;
		int ii = 0;
		for(int i = 0; i != idt.m_count; ++i, ++ii)
		{
			pe_string dll;
			bool const dll_parsed = pe_parse_import_dll_name(file_data, file_size, idt.m_table[i], dll);
			WARN_M_R(dll_parsed, L"Failed to parse import DLL name.", false);
			pe_import_address_table iat;
			bool const iat_parsed = pe_parse_import_address_table(file_data, file_size, idt.m_table[i], iat);
			WARN_M_R(iat_parsed, L"Failed to parse import address table.", false);
			ret.m_dlls[ii].m_dll_name = mm.m_strs.add_string(dll.m_str, dll.m_len, mm.m_alc);
			ret.m_dlls[ii].m_entries.resize(iat.m_count);
			for(int j = 0; j != iat.m_count; ++j)
			{
				bool is_ordinal;
				std::uint16_t ordinal;
				pe_hint_name hint_name;
				bool const ia_parsed = pe_parse_import_address(file_data, file_size, iat, j, is_ordinal, ordinal, hint_name);
				WARN_M_R(ia_parsed, L"Failed to parse address import.", false);
				ret.m_dlls[ii].m_entries[j].m_is_ordinal = is_ordinal;
				if(is_ordinal)
				{
					ret.m_dlls[ii].m_entries[j].m_ordinal_or_hint = ordinal;
				}
				else
				{
					ret.m_dlls[ii].m_entries[j].m_ordinal_or_hint = hint_name.m_hint;
					ret.m_dlls[ii].m_entries[j].m_name = mm.m_strs.add_string(hint_name.m_name.m_str, hint_name.m_name.m_len, mm.m_alc);
				}
			}
		}
		for(int i = 0; i != dlit.m_count; ++i, ++ii)
		{
			pe_string dldll;
			bool const dldll_parsed = pe_parse_delay_import_dll_name(file_data, file_size, dlit.m_table[i], dldll);
			WARN_M_R(dldll_parsed, L"Failed to parse delay load DLL name.", false);
			pe_delay_load_import_address_table dliat;
			bool const dliat_parsed = pe_parse_delay_import_address_table(file_data, file_size, dlit.m_table[i], dliat);
			WARN_M_R(dliat_parsed, L"Failed to parse delay load import address table.", false);
			ret.m_dlls[ii].m_dll_name = mm.m_strs.add_string(dldll.m_str, dldll.m_len, mm.m_alc);
			ret.m_dlls[ii].m_entries.resize(dliat.m_count);
			for(int j = 0; j != dliat.m_count; ++j)
			{
				bool is_ordinal;
				std::uint16_t ordinal;
				pe_hint_name hint_name;
				bool const dlia_parsed = pe_parse_delay_import_address(file_data, file_size, dlit.m_table[i], dliat, j, is_ordinal, ordinal, hint_name);
				WARN_M_R(dlia_parsed, L"Failed to parse delay load address import.", false);
				ret.m_dlls[ii].m_entries[j].m_is_ordinal = is_ordinal;
				if(is_ordinal)
				{
					ret.m_dlls[ii].m_entries[j].m_ordinal_or_hint = ordinal;
				}
				else
				{
					ret.m_dlls[ii].m_entries[j].m_ordinal_or_hint = hint_name.m_hint;
					ret.m_dlls[ii].m_entries[j].m_name = mm.m_strs.add_string(hint_name.m_name.m_str, hint_name.m_name.m_len, mm.m_alc);
				}
			}
		}
		return true;
	};
	if(!fn())
	{
		VERIFY(false);
	}
	return ret;
}

pe_export_table_info pe_process_export_table(void const* const fd, int const fs, pe_header_info const& hi, memory_manager& mm)
{
	char const* const file_data = static_cast<char const*>(fd);
	std::uint32_t const file_size = static_cast<std::uint32_t>(fs);
	pe_export_table_info ret;

	data_directory const* const dta_dir_table = reinterpret_cast<data_directory const*>(file_data + hi.m_data_directory_start);

	std::uint32_t const export_directory_rva = dta_dir_table[static_cast<int>(data_directory_type::export_table)].m_rva;
	std::uint32_t const export_directory_size = dta_dir_table[static_cast<int>(data_directory_type::export_table)].m_size;
	if(export_directory_size == 0)
	{
		return ret;
	}

	auto const export_directory_sect_off = convert_rva_to_disk_ptr(export_directory_rva, hi);
	section_header const& export_directory_section = *export_directory_sect_off.first;
	std::uint32_t const& export_directory_disk_offset = export_directory_sect_off.second;
	VERIFY(export_directory_disk_offset + export_directory_size <= export_directory_section.m_raw_ptr + export_directory_section.m_raw_size);
	VERIFY(export_directory_size >= sizeof(export_directory));
	export_directory const& export_dir = *reinterpret_cast<export_directory const*>(file_data + export_directory_disk_offset);
	VERIFY(export_dir.m_ordinal_base <= 0xFFFF);
	VERIFY(export_dir.m_export_address_count <= 0xFFFF);
	VERIFY(export_dir.m_ordinal_base + export_dir.m_export_address_count <= 0xFFFF);
	VERIFY(export_dir.m_names_count <= export_dir.m_export_address_count);
	if(export_dir.m_export_address_count == 0)
	{
		return ret;
	}

	// 64k export names should be enough for everybody.
	VERIFY(export_dir.m_names_count <= 64 * 1024);
	std::uint32_t const* export_name_pointer_table = nullptr;
	std::uint16_t const* export_ordinal_table = nullptr;
	if(export_dir.m_export_name_table_rva != 0)
	{
		auto const export_name_pointer_table_sect_off = convert_rva_to_disk_ptr(export_dir.m_export_name_table_rva, hi);
		section_header const& export_name_pointer_table_sect = *export_name_pointer_table_sect_off.first;
		std::uint32_t const& export_name_pointer_table_disk_offset = export_name_pointer_table_sect_off.second;
		VERIFY(export_name_pointer_table_sect.m_raw_ptr + export_name_pointer_table_sect.m_raw_size - export_name_pointer_table_disk_offset >= export_dir.m_names_count * sizeof(std::uint32_t));
		export_name_pointer_table = reinterpret_cast<std::uint32_t const*>(file_data + export_name_pointer_table_disk_offset);
		auto const export_ordinal_table_sect_off = convert_rva_to_disk_ptr(export_dir.m_ordinal_table_rva, hi);
		section_header const& export_ordinal_table_sect = *export_ordinal_table_sect_off.first;
		std::uint32_t const& export_ordinal_table_disk_offset = export_ordinal_table_sect_off.second;
		VERIFY(export_ordinal_table_sect.m_raw_ptr + export_ordinal_table_sect.m_raw_size - export_ordinal_table_disk_offset >= export_dir.m_names_count * sizeof(std::uint16_t));
		export_ordinal_table = reinterpret_cast<std::uint16_t const*>(file_data + export_ordinal_table_disk_offset);
	}

	// 64k exports should be enough for everybody.
	VERIFY(export_dir.m_export_address_count <= 64 * 1024);
	auto const export_address_table_sect_off = convert_rva_to_disk_ptr(export_dir.m_export_address_table_rva, hi);
	section_header const& export_address_table_section = *export_address_table_sect_off.first;
	std::uint32_t const& export_address_table_disk_off = export_address_table_sect_off.second;
	VERIFY(export_address_table_section.m_raw_ptr + export_address_table_section.m_raw_size - export_address_table_disk_off >= export_dir.m_export_address_count * sizeof(std::uint32_t));
	std::uint32_t const* export_address_table = reinterpret_cast<std::uint32_t const*>(file_data + export_address_table_disk_off);
	int const export_address_count_proper = static_cast<int>(std::count_if(export_address_table, export_address_table + export_dir.m_export_address_count, [](std::uint32_t const& e){ return e != 0; }));
	ret.m_export_address_table.resize(export_address_count_proper);
	ret.m_enpt_eot.resize(export_dir.m_names_count);
	std::fill(ret.m_enpt_eot.begin(), ret.m_enpt_eot.end(), std::make_pair(std::uint16_t(0xffff), std::uint16_t(0xffff)));
	ret.m_ordinal_base = static_cast<std::uint16_t>(export_dir.m_ordinal_base);
	std::uint16_t j = 0;
	int hint_idx = 0;
	std::uint16_t const n = static_cast<std::uint16_t>(export_dir.m_export_address_count);
	for(std::uint16_t i = 0; i != n; ++i)
	{
		std::uint32_t const export_rva = export_address_table[i];
		if(export_rva == 0)
		{
			continue;
		}
		std::uint16_t const ordinal = i + static_cast<std::uint16_t>(export_dir.m_ordinal_base);
		std::uint16_t hint = 0;
		char const* export_address_name = nullptr;
		std::uint32_t export_address_name_len = 0;
		if(export_ordinal_table)
		{
			auto const it = std::find(export_ordinal_table, export_ordinal_table + export_dir.m_names_count, i);
			if(it != export_ordinal_table + export_dir.m_names_count)
			{
				hint = static_cast<std::uint16_t>(it - export_ordinal_table);
				std::uint32_t const export_address_name_rva = export_name_pointer_table[hint];
				auto const export_address_name_sect_off = convert_rva_to_disk_ptr(export_address_name_rva, hi);
				section_header const& export_address_name_sect = *export_address_name_sect_off.first;
				std::uint32_t const& export_address_name_disk_offset = export_address_name_sect_off.second;
				export_address_name = reinterpret_cast<char const*>(file_data + export_address_name_disk_offset);
				// 32k export name length should be enough for everybody.
				std::uint32_t const export_address_name_len_max = std::min<std::uint32_t>(32 * 1024, export_address_name_sect.m_raw_ptr + export_address_name_sect.m_raw_size - export_address_name_disk_offset);
				auto const export_address_name_end = std::find(export_address_name, export_address_name + export_address_name_len_max, L'\0');
				VERIFY(export_address_name_end != export_address_name + export_address_name_len_max);
				export_address_name_len = static_cast<std::uint32_t>(export_address_name_end - export_address_name);
				VERIFY(export_address_name_len > 0);
			}
		}
		if(export_rva >= export_directory_rva && export_rva < export_directory_rva + export_directory_size)
		{
			// 32k export forwarder name length should be enough for everybody.
			auto const forwarder_name_sect_off = convert_rva_to_disk_ptr(export_rva, hi);
			section_header const& forwarder_name_sect = *forwarder_name_sect_off.first;
			std::uint32_t const& forwarder_name_dsk = forwarder_name_sect_off.second;
			char const* const forwarder_name = reinterpret_cast<char const*>(file_data + forwarder_name_dsk);
			std::uint32_t const forwarder_name_len_max = std::min<std::uint32_t>(32 * 1024, forwarder_name_sect.m_raw_ptr + forwarder_name_sect.m_raw_size - forwarder_name_dsk);
			char const* const forwarder_name_end = std::find(forwarder_name, forwarder_name + forwarder_name_len_max, '\0');
			VERIFY(forwarder_name_end != forwarder_name + forwarder_name_len_max);
			std::uint32_t const forwarder_name_len = static_cast<std::uint32_t>(forwarder_name_end - forwarder_name);
			VERIFY(forwarder_name_len >= 3);
			VERIFY(is_ascii(forwarder_name, forwarder_name_len));
			VERIFY(std::find(forwarder_name, forwarder_name_end, '.') != forwarder_name_end);
			ret.m_export_address_table[j].m_is_rva = false;
			ret.m_export_address_table[j].rva_or_forwarder.m_forwarder = mm.m_strs.add_string(forwarder_name, forwarder_name_len, mm.m_alc);
			ret.m_export_address_table[j].m_ordinal = ordinal;
			ret.m_export_address_table[j].m_debug_name = nullptr;
		}
		else
		{
			ret.m_export_address_table[j].m_is_rva = true;
			ret.m_export_address_table[j].rva_or_forwarder.m_rva = export_rva;
			ret.m_export_address_table[j].m_ordinal = ordinal;
			ret.m_export_address_table[j].m_debug_name = nullptr;
		}
		if(export_address_name)
		{
			string const* const export_name = mm.m_strs.add_string(export_address_name, export_address_name_len, mm.m_alc);
			ret.m_export_address_table[j].m_hint = hint;
			ret.m_export_address_table[j].m_name = export_name;
			VERIFY(ret.m_enpt_eot[hint].first == 0xffff && ret.m_enpt_eot[hint].second == 0xffff);
			ret.m_enpt_eot[hint].first = j;
			ret.m_enpt_eot[hint].second = i;
			++hint_idx;
		}
		else
		{
			ret.m_export_address_table[j].m_hint = 0;
			ret.m_export_address_table[j].m_name = nullptr;
		}
		++j;
	}
	VERIFY(hint_idx == static_cast<int>(export_dir.m_names_count));
	VERIFY(std::is_sorted(ret.m_enpt_eot.cbegin(), ret.m_enpt_eot.cend(), [&](auto const& a, auto const& b)
	{
		auto const& aa = ret.m_export_address_table[a.first].m_name;
		auto const& bb = ret.m_export_address_table[b.first].m_name;
		assert(aa);
		assert(bb);
		return string_less{}(aa, bb);
	}));

	return ret;
}

using resource_visitor_t = void(*)
(
	void* const& self,
	pe_resource_name_string_or_id_internal const& type_string,
	pe_resource_name_string_or_id_internal const& name_string,
	pe_resource_name_string_or_id_internal const& lang_string,
	char const* const& res_data,
	std::uint32_t const& res_data_size,
	std::uint32_t const& code_page
);

void pe_process_resource_table(void const* const fd, int const fs, pe_header_info const& hi, memory_manager& /*mm*/, resource_visitor_t const& visitor, void* const& self)
{
	char const* const file_data = static_cast<char const*>(fd);
	std::uint32_t const file_size = static_cast<std::uint32_t>(fs);

	data_directory const* const dta_dir_table = reinterpret_cast<data_directory const*>(file_data + hi.m_data_directory_start);
	std::uint32_t const resource_directory_rva = dta_dir_table[static_cast<int>(data_directory_type::resource_table)].m_rva;
	std::uint32_t const resource_directory_size = dta_dir_table[static_cast<int>(data_directory_type::resource_table)].m_size;
	if(resource_directory_size == 0)
	{
		return;
	}

	auto const resource_directory_sect_off = convert_rva_to_disk_ptr(resource_directory_rva, hi);
	section_header const& resource_directory_section = *resource_directory_sect_off.first;
	std::uint32_t const& resource_directory_disk_offset = resource_directory_sect_off.second;
	VERIFY(resource_directory_disk_offset + resource_directory_size <= resource_directory_section.m_raw_ptr + resource_directory_section.m_raw_size);

	std::uint32_t const resource_dir_type_off = 0;
	VERIFY(resource_directory_size >= resource_dir_type_off + sizeof(resource_directory_table));
	resource_directory_table const& resource_dir_type = *reinterpret_cast<resource_directory_table const*>(file_data + resource_directory_disk_offset + resource_dir_type_off);
	WARN(resource_dir_type.m_characteristics == 0);
	VERIFY(static_cast<std::uint32_t>(resource_dir_type.m_number_of_name_entries) + static_cast<std::uint32_t>(resource_dir_type.m_number_of_id_entries) <= 0xFFFF);
	std::uint16_t const resource_dir_type_count = resource_dir_type.m_number_of_name_entries + resource_dir_type.m_number_of_id_entries;
	VERIFY(resource_directory_size >= resource_dir_type_off + sizeof(resource_directory_table) + resource_dir_type_count * sizeof(resource_directory_entry));
	resource_directory_entry const* const resource_table_types = reinterpret_cast<resource_directory_entry const*>(file_data + resource_directory_disk_offset + resource_dir_type_off + sizeof(resource_directory_table));
	for(std::uint16_t i = 0; i != resource_dir_type_count; ++i)
	{
		VERIFY((resource_table_types[i].m_subdirectory_offset & (1u << 31)) != 0);
		bool const is_type_string = i < resource_dir_type.m_number_of_name_entries;
		pe_resource_name_string_or_id_internal const type_string = pe_resources_process_string(file_data, resource_directory_disk_offset, resource_directory_size, is_type_string, resource_table_types[i]);
		std::uint32_t const resource_dir_name_off = resource_table_types[i].m_subdirectory_offset &~ (1u << 31);
		VERIFY(resource_directory_size >= resource_dir_name_off + sizeof(resource_directory_table));
		resource_directory_table const& resource_dir_name = *reinterpret_cast<resource_directory_table const*>(file_data + resource_directory_disk_offset + resource_dir_name_off);
		WARN(resource_dir_name.m_characteristics == 0);
		VERIFY(static_cast<std::uint32_t>(resource_dir_name.m_number_of_name_entries) + static_cast<std::uint32_t>(resource_dir_name.m_number_of_id_entries) <= 0xFFFF);
		std::uint16_t const resource_dir_name_count = resource_dir_name.m_number_of_name_entries + resource_dir_name.m_number_of_id_entries;
		VERIFY(resource_directory_size >= resource_dir_name_off + sizeof(resource_directory_table) + resource_dir_name_count * sizeof(resource_directory_entry));
		resource_directory_entry const* const resource_table_names = reinterpret_cast<resource_directory_entry const*>(file_data + resource_directory_disk_offset + resource_dir_name_off + sizeof(resource_directory_table));
		for(std::uint16_t j = 0; j != resource_dir_name_count; ++j)
		{
			VERIFY((resource_table_names[j].m_subdirectory_offset & (1u << 31)) != 0);
			bool const is_name_string = j < resource_dir_name.m_number_of_name_entries;
			pe_resource_name_string_or_id_internal const name_string = pe_resources_process_string(file_data, resource_directory_disk_offset, resource_directory_size, is_name_string, resource_table_names[j]);
			std::uint32_t const resource_dir_lang_off = resource_table_names[j].m_subdirectory_offset &~ (1u << 31);
			VERIFY(resource_directory_size >= resource_dir_lang_off + sizeof(resource_directory_table));
			resource_directory_table const& resource_dir_lang = *reinterpret_cast<resource_directory_table const*>(file_data + resource_directory_disk_offset + resource_dir_lang_off);
			WARN(resource_dir_lang.m_characteristics == 0);
			VERIFY(static_cast<std::uint32_t>(resource_dir_lang.m_number_of_name_entries) + static_cast<std::uint32_t>(resource_dir_lang.m_number_of_id_entries) <= 0xFFFF);
			std::uint16_t const resource_dir_lang_count = resource_dir_lang.m_number_of_name_entries + resource_dir_lang.m_number_of_id_entries;
			VERIFY(resource_directory_size >= resource_dir_lang_off + sizeof(resource_directory_table) + resource_dir_lang_count * sizeof(resource_directory_entry));
			resource_directory_entry const* const resource_table_langs = reinterpret_cast<resource_directory_entry const*>(file_data + resource_directory_disk_offset + resource_dir_lang_off + sizeof(resource_directory_table));
			for(std::uint16_t k = 0; k != resource_dir_lang_count; ++k)
			{
				VERIFY((resource_table_langs[k].m_data_entry_offset & (1u << 31)) == 0);
				bool const is_lang_string = k < resource_dir_lang.m_number_of_name_entries;
				pe_resource_name_string_or_id_internal const lang_string = pe_resources_process_string(file_data, resource_directory_disk_offset, resource_directory_size, is_lang_string, resource_table_langs[k]);
				std::uint32_t const resource_dir_data_off = resource_table_langs[k].m_data_entry_offset;
				VERIFY(resource_directory_size >= resource_dir_data_off + sizeof(resource_directory_table));
				resource_data_entry const& resource_leaf_data = *reinterpret_cast<resource_data_entry const*>(file_data + resource_directory_disk_offset + resource_dir_data_off);
				WARN(resource_leaf_data.m_reserved == 0);
				auto const data_section_and_offset = convert_rva_to_disk_ptr(resource_leaf_data.m_data_rva, hi);
				section_header const& data_sct = *data_section_and_offset.first;
				std::uint32_t const& data_dsk = data_section_and_offset.second;
				VERIFY(file_size >= data_dsk);
				VERIFY(file_size >= data_dsk + resource_leaf_data.m_size);
				VERIFY(data_sct.m_raw_size >= resource_leaf_data.m_size);
				char const* const res_data = file_data + data_dsk;
				(*visitor)(self, type_string, name_string, lang_string, res_data, resource_leaf_data.m_size, resource_leaf_data.m_code_page);
			}
		}
	}
}

void resource_visitor_1
(
	void* const& self,
	pe_resource_name_string_or_id_internal const& /*type_string*/,
	pe_resource_name_string_or_id_internal const& /*name_string*/,
	pe_resource_name_string_or_id_internal const& /*lang_string*/,
	char const* const& /*res_data*/,
	std::uint32_t const& /*res_data_size*/,
	std::uint32_t const& /*code_page*/
)
{
	int& resource_count = *reinterpret_cast<int*>(self);
	++resource_count;
}

void resource_visitor_2
(
	void* const& self,
	pe_resource_name_string_or_id_internal const& type_string,
	pe_resource_name_string_or_id_internal const& name_string,
	pe_resource_name_string_or_id_internal const& lang_string,
	char const* const& res_data,
	std::uint32_t const& res_data_size,
	std::uint32_t const& code_page
)
{
	std::pair<pe_resources_table_info&, memory_manager&>& visitor_2_data = *reinterpret_cast<std::pair<pe_resources_table_info&, memory_manager&>*>(self);
	pe_resources_table_info& ret = visitor_2_data.first;
	memory_manager& mm = visitor_2_data.second;
	pe_resource resource;
	resource.m_type = convert_pe_string_to_string(type_string, mm);
	resource.m_name = convert_pe_string_to_string(name_string, mm);
	resource.m_lang = convert_pe_string_to_string(lang_string, mm);
	resource.m_data = res_data;
	resource.m_size = res_data_size;
	resource.m_code_page = code_page;
	ret.m_resources.push_back(resource);
}

pe_resources_table_info pe_process_resource_table(void const* const fd, int const fs, pe_header_info const& hi, memory_manager& mm)
{
	pe_resources_table_info ret;
	int resource_count = 0;
	pe_process_resource_table(fd, fs, hi, mm, &resource_visitor_1, &resource_count);
	ret.m_resources.reserve(resource_count);
	std::pair<pe_resources_table_info&, memory_manager&> visitor_2_data(ret, mm);
	pe_process_resource_table(fd, fs, hi, mm, &resource_visitor_2, &visitor_2_data);
	return ret;
}


std::pair<section_header const*, std::uint32_t> convert_rva_to_disk_ptr(std::uint32_t const rva, pe_header_info const& hi, section_header const* const s /* = nullptr */)
{
	section_header const* section = nullptr;
	if(s)
	{
		section = s;
		VERIFY(rva >= section->m_virtual_address && rva < section->m_virtual_address + section->m_virtual_size);
	}
	else
	{
		for(std::uint32_t i = 0; i != hi.m_section_count; ++i)
		{
			section_header const* const ss = reinterpret_cast<section_header const*>(hi.m_file_data + hi.m_section_headers_start) + i;
			if(rva >= ss->m_virtual_address && rva < ss->m_virtual_address + ss->m_raw_size)
			{
				section = ss;
				break;
			}
			else if(rva >= ss->m_virtual_address && rva >= ss->m_virtual_address + ss->m_raw_size && rva < ss->m_virtual_address + ss->m_virtual_size)
			{
				// Object is located inside region which is not stored on disk, but only in memory, it is initialized to zero at load-time.
				VERIFY(false);
			}
		}
		VERIFY(section);
	}
	return {section, (rva - section->m_virtual_address) + section->m_raw_ptr};
}


pe_resource_name_string_or_id_internal pe_resources_process_string(char const* const& file_data, std::uint32_t const& resource_directory_disk_offset, std::uint32_t const& resource_directory_size, bool const& is_string, resource_directory_entry const& dir_entry)
{
	pe_resource_name_string_or_id_internal ret;
	ret.m_is_string = is_string;
	if(is_string)
	{
		VERIFY((dir_entry.m_name_offset & (1u << 31)) != 0);
		std::uint32_t const string_off = dir_entry.m_name_offset &~ (1u << 31);
		VERIFY(resource_directory_size >= string_off + sizeof(resource_directory_string));
		VERIFY(string_off % sizeof(std::uint16_t) == 0);
		VERIFY(string_off % sizeof(wchar_t) == 0);
		resource_directory_string const& res_name_string = *reinterpret_cast<resource_directory_string const*>(file_data + resource_directory_disk_offset + string_off);
		VERIFY(res_name_string.m_length >= 1);
		VERIFY(resource_directory_size >= string_off + sizeof(resource_directory_string::m_length) + res_name_string.m_length * sizeof(char));
		ret.m_string_or_id.m_string_with_length.m_string_len = res_name_string.m_length;
		ret.m_string_or_id.m_string_with_length.m_string = reinterpret_cast<wchar_t const*>(file_data + resource_directory_disk_offset + string_off + sizeof(resource_directory_string::m_length));
	}
	else
	{
		VERIFY((dir_entry.m_name_offset & (1u << 31)) == 0);
		ret.m_string_or_id.m_id = dir_entry.m_integer_id;
	}
	return ret;
}

pe_resource_string_or_id convert_pe_string_to_string(pe_resource_name_string_or_id_internal const& str, memory_manager& mm)
{
	pe_resource_string_or_id ret;
	ret.m_is_string = str.m_is_string;
	if(str.m_is_string)
	{
		ret.m_string = mm.m_wstrs.add_string(str.m_string_or_id.m_string_with_length.m_string, str.m_string_or_id.m_string_with_length.m_string_len, mm.m_alc);
	}
	else
	{
		ret.m_id = str.m_string_or_id.m_id;
	}
	return ret;
}
