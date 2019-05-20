#include "pe.h"

#include "memory_manager.h"
#include "unicode.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cassert>


struct dos_header
{
	std::uint16_t m_signature;
	std::uint16_t m_last_size;
	std::uint16_t m_pages;
	std::uint16_t m_relocations;
	std::uint16_t m_header_size;
	std::uint16_t m_min_alloc;
	std::uint16_t m_max_alloc;
	std::uint16_t m_ss;
	std::uint16_t m_sp;
	std::uint16_t m_check_sum;
	std::uint16_t m_ip;
	std::uint16_t m_cs;
	std::uint16_t m_relocs;
	std::uint16_t m_overlay;
	std::uint16_t m_reserved_1[4];
	std::uint16_t m_oem_id;
	std::uint16_t m_oem_info;
	std::uint16_t m_reserved_2[10];
	std::uint16_t m_pe_offset;
};
static_assert(sizeof(dos_header) == 62, "");
static_assert(sizeof(dos_header) == 0x3e, "");

struct coff_header
{
	std::uint32_t m_signature;
	std::uint16_t m_machine;
	std::uint16_t m_section_count;
	std::uint32_t m_date_time;
	std::uint32_t m_symbol_table;
	std::uint32_t m_symbol_table_entries_count;
	std::uint16_t m_optional_header_size;
	std::uint16_t m_characteristics;
};
static_assert(sizeof(coff_header) == 24, "");
static_assert(sizeof(coff_header) == 0x18, "");

struct coff_optional_header_pe32_standard
{
	std::uint16_t m_signature;
	std::uint8_t m_linker_major;
	std::uint8_t m_linker_minor;
	std::uint32_t m_code_size;
	std::uint32_t m_initialized_size;
	std::uint32_t m_uninitialized_size;
	std::uint32_t m_entry_point;
	std::uint32_t m_code_base;
	std::uint32_t m_data_base;
};
static_assert(sizeof(coff_optional_header_pe32_standard) == 28, "");
static_assert(sizeof(coff_optional_header_pe32_standard) == 0x1c, "");

struct coff_optional_header_pe32_plus_standard
{
	std::uint16_t m_signature;
	std::uint8_t m_linker_major;
	std::uint8_t m_linker_minor;
	std::uint32_t m_code_size;
	std::uint32_t m_initialized_size;
	std::uint32_t m_uninitialized_size;
	std::uint32_t m_entry_point;
	std::uint32_t m_code_base;
};
static_assert(sizeof(coff_optional_header_pe32_plus_standard) == 24, "");
static_assert(sizeof(coff_optional_header_pe32_plus_standard) == 0x18, "");

struct coff_optional_header_pe32_windows_specific
{
	std::uint32_t m_image_base;
	std::uint32_t m_section_alignment;
	std::uint32_t m_file_alignment;
	std::uint16_t m_os_major;
	std::uint16_t m_os_minor;
	std::uint16_t m_image_major;
	std::uint16_t m_image_minor;
	std::uint16_t m_subsystem_major;
	std::uint16_t m_subsystem_minor;
	std::uint32_t m_win32version;
	std::uint32_t m_image_size;
	std::uint32_t m_headers_size;
	std::uint32_t m_check_sum;
	std::uint16_t m_subsystem;
	std::uint16_t m_dll_characteristics;
	std::uint32_t m_stack_reserve;
	std::uint32_t m_stack_commit;
	std::uint32_t m_heap_reserve;
	std::uint32_t m_heap_commit;
	std::uint32_t m_loader_flags;
	std::uint32_t m_data_directory_count;
};
static_assert(sizeof(coff_optional_header_pe32_windows_specific) == 68, "");
static_assert(sizeof(coff_optional_header_pe32_windows_specific) == 0x44, "");

struct coff_optional_header_pe32_plus_windows_specific
{
	std::uint64_t m_image_base;
	std::uint32_t m_section_alignment;
	std::uint32_t m_file_alignment;
	std::uint16_t m_os_major;
	std::uint16_t m_os_minor;
	std::uint16_t m_image_major;
	std::uint16_t m_image_minor;
	std::uint16_t m_subsystem_major;
	std::uint16_t m_subsystem_minor;
	std::uint32_t m_win32version;
	std::uint32_t m_image_size;
	std::uint32_t m_headers_size;
	std::uint32_t m_check_sum;
	std::uint16_t m_subsystem;
	std::uint16_t m_dll_characteristics;
	std::uint64_t m_stack_reserve;
	std::uint64_t m_stack_commit;
	std::uint64_t m_heap_reserve;
	std::uint64_t m_heap_commit;
	std::uint32_t m_loader_flags;
	std::uint32_t m_data_directory_count;
};
static_assert(sizeof(coff_optional_header_pe32_plus_windows_specific) == 88, "");
static_assert(sizeof(coff_optional_header_pe32_plus_windows_specific) == 0x58, "");

struct coff_optional_header_pe32
{
	coff_optional_header_pe32_standard m_standard;
	coff_optional_header_pe32_windows_specific m_windows;
};
static_assert(sizeof(coff_optional_header_pe32) == sizeof(coff_optional_header_pe32_standard) + sizeof(coff_optional_header_pe32_windows_specific), "");
static_assert(sizeof(coff_optional_header_pe32) == 96, "");
static_assert(sizeof(coff_optional_header_pe32) == 0x60, "");

struct coff_optional_header_pe32_plus
{
	coff_optional_header_pe32_plus_standard m_standard;
	coff_optional_header_pe32_plus_windows_specific m_windows;
};
static_assert(sizeof(coff_optional_header_pe32_plus) == sizeof(coff_optional_header_pe32_plus_standard) + sizeof(coff_optional_header_pe32_plus_windows_specific), "");
static_assert(sizeof(coff_optional_header_pe32_plus) == 112, "");
static_assert(sizeof(coff_optional_header_pe32_plus) == 0x70, "");

struct coff_entie_header_pe32
{
	coff_header m_mandatory_header;
	coff_optional_header_pe32 m_optional_header;
};
static_assert(sizeof(coff_entie_header_pe32) == sizeof(coff_header) + sizeof(coff_optional_header_pe32), "");
static_assert(sizeof(coff_entie_header_pe32) == 120, "");
static_assert(sizeof(coff_entie_header_pe32) == 0x78, "");

struct coff_entie_header_pe32_plus
{
	coff_header m_mandatory_header;
	coff_optional_header_pe32_plus m_optional_header;
};
static_assert(sizeof(coff_entie_header_pe32_plus) == sizeof(coff_header) + sizeof(coff_optional_header_pe32_plus), "");
static_assert(sizeof(coff_entie_header_pe32_plus) == 136, "");
static_assert(sizeof(coff_entie_header_pe32_plus) == 0x88, "");

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

struct import_directory_entry
{
	std::uint32_t m_import_lookup_table;
	std::uint32_t m_date_time;
	std::uint32_t m_forwarder_chain;
	std::uint32_t m_name;
	std::uint32_t m_import_adress_table;
};
static_assert(sizeof(import_directory_entry) == 20, "");
static_assert(sizeof(import_directory_entry) == 0x14, "");

struct import_lookup_entry_pe32
{
	std::uint32_t m_value;
};
static_assert(sizeof(import_lookup_entry_pe32) == 4, "");
static_assert(sizeof(import_lookup_entry_pe32) == 0x4, "");

struct import_lookup_entry_pe32_plus
{
	std::uint64_t m_value;
};
static_assert(sizeof(import_lookup_entry_pe32_plus) == 8, "");
static_assert(sizeof(import_lookup_entry_pe32_plus) == 0x8, "");

struct import_hint_name
{
	std::uint16_t m_hint;
	char m_name[2];
};
static_assert(sizeof(import_hint_name) == 4, "");
static_assert(sizeof(import_hint_name) == 0x4, "");

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


static constexpr wchar_t const s_bad_format[] = L"Bad format.";


#define VERIFY(X) do{ if(!(X)){ throw s_bad_format; } }while(false)


pe_header_info pe_process_header(void const* const fd, int const fs)
{
	char const* const file_data = static_cast<char const*>(fd);
	std::uint32_t const file_size = static_cast<std::uint32_t>(fs);

	static constexpr uint16_t const s_mz_signature = 0x5a4d; // MZ
	VERIFY(file_size >= sizeof(dos_header));
	dos_header const& dos_hdr = *reinterpret_cast<dos_header const*>(file_data);
	VERIFY(dos_hdr.m_signature == s_mz_signature);

	static constexpr std::uint32_t const s_pe_signature_len = 4;
	static constexpr uint32_t const s_pe_signature = 0x00004550; // PE\0\0
	VERIFY(file_size >= dos_hdr.m_pe_offset + s_pe_signature_len);
	std::uint32_t const& pe_signature = *reinterpret_cast<std::uint32_t const*>(file_data + dos_hdr.m_pe_offset);
	VERIFY(pe_signature == s_pe_signature);

	static constexpr uint16_t const s_machine_type_386 = 0x014c;
	static constexpr uint16_t const s_machine_type_amd64 = 0x8664;
	static constexpr uint16_t const s_max_coff_header_sections = 96;
	VERIFY(file_size >= dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header));
	coff_header const& coff_hdr = *reinterpret_cast<coff_header const*>(file_data + dos_hdr.m_pe_offset);
	VERIFY(coff_hdr.m_machine == s_machine_type_386 || coff_hdr.m_machine == s_machine_type_amd64);
	VERIFY(coff_hdr.m_section_count <= s_max_coff_header_sections);

	static constexpr int const s_coff_optional_header_signature_len = 2;
	static constexpr uint16_t const s_coff_optional_type_pe32 = 0x010b;
	static constexpr uint16_t const s_coff_optional_type_pe32_plus = 0x020b;
	VERIFY(coff_hdr.m_optional_header_size >= s_coff_optional_header_signature_len);
	VERIFY(file_size >= dos_hdr.m_pe_offset + s_pe_signature_len + sizeof(coff_header) + s_coff_optional_header_signature_len);
	std::uint16_t const& coff_optional_header_signature = *reinterpret_cast<std::uint16_t const*>(file_data + dos_hdr.m_pe_offset + sizeof(coff_header));
	VERIFY(coff_optional_header_signature == s_coff_optional_type_pe32 || coff_optional_header_signature == s_coff_optional_type_pe32_plus);
	bool const is_pe32 = coff_optional_header_signature == s_coff_optional_type_pe32;

	VERIFY(coff_hdr.m_optional_header_size >= (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)));
	VERIFY(file_size >= dos_hdr.m_pe_offset + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)));
	coff_optional_header_pe32 const& coff_hdr_opt_pe32 = *reinterpret_cast<coff_optional_header_pe32 const*>(file_data + dos_hdr.m_pe_offset + sizeof(coff_header));
	coff_optional_header_pe32_plus const& coff_hdr_opt_pe32_plus = *reinterpret_cast<coff_optional_header_pe32_plus const*>(file_data + dos_hdr.m_pe_offset + sizeof(coff_header));
	VERIFY(((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_image_base : coff_hdr_opt_pe32_plus.m_windows.m_image_base) % (64 * 1024)) == 0);
	VERIFY(((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_section_alignment : coff_hdr_opt_pe32_plus.m_windows.m_section_alignment) % (is_pe32 ? coff_hdr_opt_pe32.m_windows.m_file_alignment : coff_hdr_opt_pe32_plus.m_windows.m_file_alignment)) == 0);
	VERIFY(((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_file_alignment : coff_hdr_opt_pe32_plus.m_windows.m_file_alignment) % (512)) == 0);
	VERIFY(((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_file_alignment : coff_hdr_opt_pe32_plus.m_windows.m_file_alignment) % (128 * 1024)) != 0);
	VERIFY(((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_section_alignment : coff_hdr_opt_pe32_plus.m_windows.m_section_alignment) >= 4 * 1024) || ((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_file_alignment : coff_hdr_opt_pe32_plus.m_windows.m_file_alignment) == (is_pe32 ? coff_hdr_opt_pe32.m_windows.m_section_alignment : coff_hdr_opt_pe32_plus.m_windows.m_section_alignment)));
	VERIFY((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_win32version : coff_hdr_opt_pe32_plus.m_windows.m_win32version) == 0);
	VERIFY(((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_image_size : coff_hdr_opt_pe32_plus.m_windows.m_image_size) % (is_pe32 ? coff_hdr_opt_pe32.m_windows.m_section_alignment : coff_hdr_opt_pe32_plus.m_windows.m_section_alignment)) == 0);
	VERIFY(((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_headers_size : coff_hdr_opt_pe32_plus.m_windows.m_headers_size) % (is_pe32 ? coff_hdr_opt_pe32.m_windows.m_file_alignment : coff_hdr_opt_pe32_plus.m_windows.m_file_alignment)) == 0);
	VERIFY((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_loader_flags : coff_hdr_opt_pe32_plus.m_windows.m_loader_flags) == 0);

	coff_entie_header_pe32 const& coff_entire_hdr = *reinterpret_cast<coff_entie_header_pe32 const*>(file_data + dos_hdr.m_pe_offset);
	coff_entie_header_pe32_plus const& coff_entire_hdr_plus = *reinterpret_cast<coff_entie_header_pe32_plus const*>(file_data + dos_hdr.m_pe_offset);

	VERIFY((is_pe32 ? coff_hdr_opt_pe32.m_windows.m_data_directory_count : coff_hdr_opt_pe32_plus.m_windows.m_data_directory_count) == 16);
	VERIFY(coff_hdr.m_optional_header_size == (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)) + sizeof(std::array<data_directory, 16>));
	VERIFY(file_size >= dos_hdr.m_pe_offset + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)) + sizeof(std::array<data_directory, 16>));
	std::array<data_directory, 16> const& data_directories = *reinterpret_cast<std::array<data_directory, 16> const*>(file_data + dos_hdr.m_pe_offset + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)));
	VERIFY(data_directories[static_cast<int>(data_directory_type::architecture)].m_rva  == 0);
	VERIFY(data_directories[static_cast<int>(data_directory_type::architecture)].m_size == 0);
	VERIFY(data_directories[static_cast<int>(data_directory_type::global_ptr)].m_size == 0);
	VERIFY(data_directories[static_cast<int>(data_directory_type::reserved)].m_rva == 0);
	VERIFY(data_directories[static_cast<int>(data_directory_type::reserved)].m_size == 0);

	VERIFY(file_size >= dos_hdr.m_pe_offset + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)) + sizeof(std::array<data_directory, 16>) + coff_hdr.m_section_count * sizeof(section_header));
	section_header const* const section_headers_begin = reinterpret_cast<section_header const*>(file_data + dos_hdr.m_pe_offset + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)) + sizeof(std::array<data_directory, 16>));
	section_header const* const section_headers_end = section_headers_begin + coff_hdr.m_section_count;
	std::uint32_t prev_va;
	std::uint32_t prev_size;
	if(coff_hdr.m_section_count > 0)
	{
		prev_va = section_headers_begin[0].m_virtual_address;
		prev_size = 0;
	}
	for(std::uint32_t i = 0; i != coff_hdr.m_section_count; ++i)
	{
		section_header const& sct_hdr = section_headers_begin[i];
		VERIFY(file_size >= sct_hdr.m_raw_ptr + sct_hdr.m_raw_size);
		VERIFY((sct_hdr.m_virtual_address % (is_pe32 ? coff_hdr_opt_pe32.m_windows.m_section_alignment : coff_hdr_opt_pe32_plus.m_windows.m_section_alignment)) == 0);
		VERIFY(sct_hdr.m_virtual_address >= prev_va + prev_size);
		prev_va = sct_hdr.m_virtual_address;
		prev_size = sct_hdr.m_virtual_size;
		VERIFY((sct_hdr.m_raw_size % (is_pe32 ? coff_hdr_opt_pe32.m_windows.m_file_alignment : coff_hdr_opt_pe32_plus.m_windows.m_file_alignment)) == 0);
		VERIFY((sct_hdr.m_raw_ptr % (is_pe32 ? coff_hdr_opt_pe32.m_windows.m_file_alignment : coff_hdr_opt_pe32_plus.m_windows.m_file_alignment)) == 0);
	}

	pe_header_info ret;
	ret.m_file_data = file_data;
	ret.m_file_size = file_size;
	ret.m_pe_header_start = dos_hdr.m_pe_offset;
	ret.m_is_pe32 = is_pe32;
	ret.m_data_directory_count = 16;
	ret.m_data_directory_start = dos_hdr.m_pe_offset + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus));
	ret.m_section_count = coff_hdr.m_section_count;
	ret.m_section_headers_start = dos_hdr.m_pe_offset + sizeof(coff_header) + (is_pe32 ? sizeof(coff_optional_header_pe32) : sizeof(coff_optional_header_pe32_plus)) + sizeof(std::array<data_directory, 16>);
	return ret;
}

pe_import_table_info pe_process_import_table(void const* const fd, int const fs, pe_header_info const& hi, memory_manager& mm)
{
	char const* const file_data = static_cast<char const*>(fd);
	std::uint32_t const file_size = static_cast<std::uint32_t>(fs);
	pe_import_table_info ret;

	data_directory const* const dta_dir_table = reinterpret_cast<data_directory const*>(file_data + hi.m_data_directory_start);

	std::uint32_t const import_table_rva = dta_dir_table[static_cast<int>(data_directory_type::import_table)].m_rva;
	std::uint32_t const import_table_size = dta_dir_table[static_cast<int>(data_directory_type::import_table)].m_size;
	if(import_table_size == 0)
	{
		return ret;
	}

	auto const import_table_section_and_offset = convert_rva_to_disk_ptr(import_table_rva, hi);
	section_header const& import_table_section = *import_table_section_and_offset.first;
	std::uint32_t const import_table_offset = import_table_section_and_offset.second;
	VERIFY(import_table_section.m_raw_ptr + import_table_section.m_raw_size >= import_table_offset + import_table_size);
	// 32k DLLs should be enough for everybody.
	std::uint32_t const import_table_max_count = std::min<std::uint32_t>(32 * 1024, import_table_size / sizeof(import_directory_entry));
	import_directory_entry const* const import_directory_table = reinterpret_cast<import_directory_entry const*>(file_data + import_table_offset);
	std::uint32_t import_directory_table_count = 0xFFFFFFFF;
	for(std::uint32_t i = 0; i != import_table_max_count; ++i)
	{
		if
		(
			import_directory_table[i].m_import_lookup_table == 0 &&
			import_directory_table[i].m_date_time == 0 &&
			import_directory_table[i].m_forwarder_chain == 0 &&
			import_directory_table[i].m_name == 0 &&
			import_directory_table[i].m_import_adress_table == 0
		)
		{
			import_directory_table_count = i;
			break;
		}
	}
	VERIFY(import_directory_table_count != 0xFFFFFFFF);
	ret.m_dlls.resize(import_directory_table_count);
	for(std::uint32_t i = 0; i != import_directory_table_count; ++i)
	{
		std::uint32_t const dll_name_rva = import_directory_table[i].m_name;
		auto const dll_name_sct_dsk = convert_rva_to_disk_ptr(dll_name_rva, hi);
		section_header const& dll_name_sct = *dll_name_sct_dsk.first;
		std::uint32_t const dll_name_dsk = dll_name_sct_dsk.second;
		std::uint32_t dll_name_len = 0xFFFFFFFF;
		// 32k DLL name should be enough for everybody.
		std::uint32_t const dll_name_len_max = std::min<std::uint32_t>(32 * 1024, dll_name_sct.m_raw_ptr + dll_name_sct.m_raw_size - dll_name_dsk);
		for(std::uint32_t i = 0; i != dll_name_len_max; ++i)
		{
			if(reinterpret_cast<char const*>(file_data + dll_name_dsk)[i] == '\0')
			{
				dll_name_len = i;
				break;
			}
		}
		VERIFY(dll_name_len != 0xFFFFFFFF && dll_name_len > 0);
		char const* const dll_name = reinterpret_cast<char const*>(file_data + dll_name_dsk);
		VERIFY(is_ascii(dll_name, static_cast<int>(dll_name_len)));
		ret.m_dlls[i].m_dll_name = mm.m_strs.add_string(dll_name, dll_name_len, mm.m_alc);
		auto const import_lookup_table_sct_dsk = convert_rva_to_disk_ptr(import_directory_table[i].m_import_lookup_table != 0 ? import_directory_table[i].m_import_lookup_table : import_directory_table[i].m_import_adress_table, hi);
		section_header const& import_lookup_table_sct = *import_lookup_table_sct_dsk.first;
		std::uint32_t const import_lookup_table_dsk = import_lookup_table_sct_dsk.second;
		std::uint32_t import_lookup_table_count = 0xFFFFFFFF;
		if(hi.m_is_pe32)
		{
			// 64k imports per DLL should be enough for everybody.
			std::uint32_t const import_lookup_table_max_count = std::min<std::uint32_t>(64 * 1024, (import_lookup_table_sct.m_raw_ptr + import_lookup_table_sct.m_raw_size - import_lookup_table_dsk) / sizeof(import_lookup_entry_pe32));
			import_lookup_entry_pe32 const* const import_lookup_table = reinterpret_cast<import_lookup_entry_pe32 const*>(file_data + import_lookup_table_dsk);
			for(std::uint32_t i = 0; i != import_lookup_table_max_count; ++i)
			{
				import_lookup_entry_pe32 const& import_entry = import_lookup_table[i];
				if(import_entry.m_value == 0u)
				{
					import_lookup_table_count = i;
					break;
				}
			}
			VERIFY(import_lookup_table_count != 0xFFFFFFFF);
			ret.m_dlls[i].m_entries.resize(import_lookup_table_count);
			for(std::uint32_t j = 0; j != import_lookup_table_count; ++j)
			{
				import_lookup_entry_pe32 const& import_entry = import_lookup_table[j];
				bool const is_ordinal = (import_entry.m_value & 0x80000000u) == 0x80000000u;
				if(is_ordinal)
				{
					VERIFY((import_entry.m_value & 0x7FFF0000u) == 0u);
					std::uint16_t const import_ordinal = import_entry.m_value & 0x0000FFFFu;
					ret.m_dlls[i].m_entries[j].m_is_ordinal = true;
					ret.m_dlls[i].m_entries[j].m_ordinal_or_hint = import_ordinal;
					ret.m_dlls[i].m_entries[j].m_name = nullptr;
				}
				else
				{
					VERIFY((import_entry.m_value & 0x80000000u) == 0u);
					std::uint32_t const hint_name_offset = import_entry.m_value &~ 0x80000000u;
					auto const hint_name_sect_off = convert_rva_to_disk_ptr(hint_name_offset, hi);
					section_header const& hint_name_section = *hint_name_sect_off.first;
					std::uint32_t const hint_name_disk_offset = hint_name_sect_off.second;
					VERIFY(file_size >= hint_name_disk_offset + sizeof(import_hint_name));
					VERIFY(hint_name_section.m_raw_size >= sizeof(import_hint_name));
					import_hint_name const& hint_name = *reinterpret_cast<import_hint_name const*>(file_data + hint_name_disk_offset);
					std::uint16_t const hint = hint_name.m_hint;
					char const* const name = hint_name.m_name;
					std::uint32_t import_name_len = 0xFFFFFFFF;
					// 32k import name length should be enough for everybody.
					std::uint32_t const import_name_len_max = std::min<std::uint32_t>(32 * 1024, hint_name_section.m_raw_ptr + hint_name_section.m_raw_size - hint_name_disk_offset - sizeof(import_hint_name::m_hint));
					for(std::uint32_t i = 0; i != import_name_len_max; ++i)
					{
						if(name[i] == '\0')
						{
							import_name_len = i;
							break;
						}
					}
					VERIFY(import_name_len != 0xFFFFFFFF && import_name_len > 0);
					VERIFY(is_ascii(name, import_name_len));
					ret.m_dlls[i].m_entries[j].m_is_ordinal = false;
					ret.m_dlls[i].m_entries[j].m_ordinal_or_hint = hint;
					ret.m_dlls[i].m_entries[j].m_name = mm.m_strs.add_string(name, import_name_len, mm.m_alc);
				}
			}
		}
		else
		{
			// 64k imports per DLL should be enough for everybody.
			std::uint32_t const import_lookup_table_max_count = std::min<std::uint32_t>(64 * 1024, (import_lookup_table_sct.m_raw_ptr + import_lookup_table_sct.m_raw_size - import_lookup_table_dsk) / sizeof(import_lookup_entry_pe32_plus));
			import_lookup_entry_pe32_plus const* const import_lookup_table = reinterpret_cast<import_lookup_entry_pe32_plus const*>(file_data + import_lookup_table_dsk);
			for(std::uint32_t i = 0; i != import_lookup_table_max_count; ++i)
			{
				import_lookup_entry_pe32_plus const& import_entry = import_lookup_table[i];
				if(import_entry.m_value == 0ull)
				{
					import_lookup_table_count = i;
					break;
				}
			}
			VERIFY(import_lookup_table_count != 0xFFFFFFFF);
			ret.m_dlls[i].m_entries.resize(import_lookup_table_count);
			for(std::uint32_t j = 0; j != import_lookup_table_count; ++j)
			{
				import_lookup_entry_pe32_plus const& import_entry = import_lookup_table[j];
				bool const is_ordinal = (import_entry.m_value & 0x8000000000000000ull) == 0x8000000000000000ull;
				if(is_ordinal)
				{
					VERIFY((import_entry.m_value & 0x7FFFFFFFFFFF0000ull) == 0ull);
					std::uint16_t const import_ordinal = import_entry.m_value & 0x000000000000FFFFull;
					ret.m_dlls[i].m_entries[j].m_is_ordinal = true;
					ret.m_dlls[i].m_entries[j].m_ordinal_or_hint = import_ordinal;
					ret.m_dlls[i].m_entries[j].m_name = nullptr;
				}
				else
				{
					VERIFY((import_entry.m_value & 0x7FFFFFFF00000000ull) == 0ull);
					std::uint32_t const hint_name_offset = import_entry.m_value &~ 0xFFFFFFFF80000000ull;
					auto const hint_name_sect_off = convert_rva_to_disk_ptr(hint_name_offset, hi);
					section_header const& hint_name_section = *hint_name_sect_off.first;
					std::uint32_t const hint_name_disk_offset = hint_name_sect_off.second;
					VERIFY(file_size >= hint_name_disk_offset + sizeof(import_hint_name));
					VERIFY(hint_name_section.m_raw_size >= sizeof(import_hint_name));
					import_hint_name const& hint_name = *reinterpret_cast<import_hint_name const*>(file_data + hint_name_disk_offset);
					std::uint16_t const hint = hint_name.m_hint;
					char const* const name = hint_name.m_name;
					std::uint32_t import_name_len = 0xFFFFFFFF;
					// 32k import name length should be enough for everybody.
					std::uint32_t const import_name_len_max = std::min<std::uint32_t>(32 * 1024, hint_name_section.m_raw_ptr + hint_name_section.m_raw_size - hint_name_disk_offset - sizeof(import_hint_name::m_hint));
					for(std::uint32_t i = 0; i != import_name_len_max; ++i)
					{
						if(name[i] == '\0')
						{
							import_name_len = i;
							break;
						}
					}
					VERIFY(import_name_len != 0xFFFFFFFF && import_name_len > 0);
					VERIFY(is_ascii(name, import_name_len));
					ret.m_dlls[i].m_entries[j].m_is_ordinal = false;
					ret.m_dlls[i].m_entries[j].m_ordinal_or_hint = hint;
					ret.m_dlls[i].m_entries[j].m_name = mm.m_strs.add_string(name, import_name_len, mm.m_alc);
				}
			}
		}
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
	std::uint32_t const export_directory_disk_offset = export_directory_sect_off.second;
	VERIFY(export_directory_disk_offset + export_directory_size <= export_directory_section.m_raw_ptr + export_directory_section.m_raw_size);
	VERIFY(export_directory_size >= sizeof(export_directory));
	export_directory const& export_dir = *reinterpret_cast<export_directory const*>(file_data + export_directory_disk_offset);
	VERIFY(export_dir.m_ordinal_base <= 0xFFFF);
	VERIFY(export_dir.m_export_address_count <= 0xFFFF);
	VERIFY(export_dir.m_ordinal_base + export_dir.m_export_address_count <= 0xFFFF);
	VERIFY(export_dir.m_names_count <= export_dir.m_export_address_count);
	VERIFY((export_dir.m_export_name_table_rva == 0 && export_dir.m_ordinal_table_rva == 0) || (export_dir.m_export_name_table_rva != 0 && export_dir.m_ordinal_table_rva != 0));

	// 64k export names should be enough for everybody.
	VERIFY(export_dir.m_names_count <= 64 * 1024);
	std::uint32_t const* export_name_pointer_table = nullptr;
	std::uint16_t const* export_ordinal_table = nullptr;
	if(export_dir.m_export_name_table_rva != 0)
	{
		std::uint32_t const export_name_pointer_table_disk_offset = convert_rva_to_disk_ptr(export_dir.m_export_name_table_rva, hi, &export_directory_section).second;
		VERIFY(export_directory_section.m_raw_ptr + export_directory_section.m_raw_size - export_name_pointer_table_disk_offset >= export_dir.m_names_count * sizeof(std::uint32_t));
		export_name_pointer_table = reinterpret_cast<std::uint32_t const*>(file_data + export_name_pointer_table_disk_offset);
		std::uint32_t const export_ordinal_table_disk_offset = convert_rva_to_disk_ptr(export_dir.m_ordinal_table_rva, hi, &export_directory_section).second;
		VERIFY(export_directory_section.m_raw_ptr + export_directory_section.m_raw_size - export_ordinal_table_disk_offset >= export_dir.m_names_count * sizeof(std::uint16_t));
		export_ordinal_table = reinterpret_cast<std::uint16_t const*>(file_data + export_ordinal_table_disk_offset);
	}

	// 64k exports should be enough for everybody.
	VERIFY(export_dir.m_export_address_count <= 64 * 1024);
	auto const export_address_table_sect_off = convert_rva_to_disk_ptr(export_dir.m_export_address_table_rva, hi, &export_directory_section);
	section_header const& export_address_table_section = *export_address_table_sect_off.first;
	std::uint32_t const export_address_table_disk_off = export_address_table_sect_off.second;
	VERIFY(export_address_table_section.m_raw_ptr + export_address_table_section.m_raw_size - export_address_table_disk_off >= export_dir.m_export_address_count * sizeof(std::uint32_t));
	std::uint32_t const* export_address_table = reinterpret_cast<std::uint32_t const*>(file_data + export_address_table_disk_off);
	int const export_address_count_proper = static_cast<int>(std::count_if(export_address_table, export_address_table + export_dir.m_export_address_count, [](std::uint32_t const& e){ return e != 0; }));
	ret.m_export_address_table.resize(export_address_count_proper);
	int j = 0;
	for(std::uint32_t i = 0; i != export_dir.m_export_address_count; ++i)
	{
		std::uint32_t const export_rva = export_address_table[i];
		if(export_rva == 0)
		{
			continue;
		}
		std::uint16_t const ordinal = i + export_dir.m_ordinal_base;
		std::uint16_t hint;
		char const* export_address_name = nullptr;
		std::uint32_t export_address_name_len;
		if(export_ordinal_table)
		{
			auto const it = std::find(export_ordinal_table, export_ordinal_table + export_dir.m_names_count, i);
			if(it != export_ordinal_table + export_dir.m_names_count)
			{
				hint = static_cast<std::uint16_t>(it - export_ordinal_table);
				std::uint32_t const export_address_name_rva = export_name_pointer_table[it - export_ordinal_table];
				std::uint32_t const export_address_name_disk_offset = convert_rva_to_disk_ptr(export_address_name_rva, hi, &export_directory_section).second;
				export_address_name = reinterpret_cast<char const*>(file_data + export_address_name_disk_offset);
				// 32k export name length should be enough for everybody.
				std::uint32_t const export_address_name_len_max = std::min<std::uint32_t>(32 * 1024, export_directory_section.m_raw_ptr + export_directory_section.m_raw_size - export_address_name_disk_offset);
				auto const export_address_name_end = std::find(export_address_name, export_address_name + export_address_name_len_max, L'\0');
				VERIFY(export_address_name_end != export_address_name + export_address_name_len_max);
				export_address_name_len = static_cast<std::uint32_t>(export_address_name_end - export_address_name);
				VERIFY(export_address_name_len > 0);
			}
		}
		if(export_rva >= export_directory_rva && export_rva < export_directory_rva + export_directory_size)
		{
			// 32k export forwarder name length should be enough for everybody.
			std::uint32_t const forwarder_name_dsk = convert_rva_to_disk_ptr(export_rva, hi, &export_directory_section).second;
			char const* const forwarder_name = reinterpret_cast<char const*>(file_data + forwarder_name_dsk);
			std::uint32_t const forwarder_name_len_max = std::min<std::uint32_t>(32 * 1024, export_directory_section.m_raw_ptr + export_directory_section.m_raw_size - forwarder_name_dsk);
			char const* const forwarder_name_end = std::find(forwarder_name, forwarder_name + forwarder_name_len_max, '\0');
			VERIFY(forwarder_name_end != forwarder_name + forwarder_name_len_max);
			std::uint32_t const forwarder_name_len = static_cast<std::uint32_t>(forwarder_name_end - forwarder_name);
			VERIFY(forwarder_name_len >= 3);
			VERIFY(is_ascii(forwarder_name, forwarder_name_len));
			VERIFY(std::find(forwarder_name, forwarder_name_end, '.') != forwarder_name_end);
			ret.m_export_address_table[j].m_is_rva = false;
			ret.m_export_address_table[j].m_ordinal = ordinal;
			ret.m_export_address_table[j].m_rva = 0;
			ret.m_export_address_table[j].m_forwarder = mm.m_strs.add_string(forwarder_name, forwarder_name_len, mm.m_alc);
		}
		else
		{
			ret.m_export_address_table[j].m_is_rva = true;
			ret.m_export_address_table[j].m_ordinal = ordinal;
			ret.m_export_address_table[j].m_rva = export_rva;
			ret.m_export_address_table[j].m_forwarder = nullptr;
		}
		if(export_address_name)
		{
			ret.m_export_address_table[j].m_hint = hint;
			ret.m_export_address_table[j].m_name = mm.m_strs.add_string(export_address_name, export_address_name_len, mm.m_alc);
		}
		else
		{
			ret.m_export_address_table[j].m_hint = 0;
			ret.m_export_address_table[j].m_name = nullptr;
		}
		++j;
	}

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
			if(rva >= ss->m_virtual_address && rva < ss->m_virtual_address + ss->m_virtual_size)
			{
				section = ss;
				break;
			}
		}
		VERIFY(section);
	}
	return {section, (rva - section->m_virtual_address) + section->m_raw_ptr};
}
