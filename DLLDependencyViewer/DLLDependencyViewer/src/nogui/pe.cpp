#include "pe.h"

#include "unicode.h"

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


static constexpr char const s_bad_format[] = "Bad format.";


#define VERIFY(X) do{ assert(X); if(!(X)){ throw s_bad_format; } }while(false)


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
		//VERIFY(sct_hdr.m_virtual_size >= sct_hdr.m_raw_size); // Most of the times, except size on disk is rounded but size in memory is not.
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

pe_import_table_info pe_process_import_table(void const* const fd, int const fs, pe_header_info const& hi)
{
	char const* const file_data = static_cast<char const*>(fd);
	std::uint32_t const file_size = static_cast<std::uint32_t>(fs);
	pe_import_table_info ret;

	data_directory const* const dta_dir_table = reinterpret_cast<data_directory const*>(file_data + hi.m_data_directory_start);

	std::uint32_t const import_table_addr = dta_dir_table[static_cast<int>(data_directory_type::import_table)].m_rva;
	std::uint32_t const import_table_size = dta_dir_table[static_cast<int>(data_directory_type::import_table)].m_size;
	if(import_table_size == 0)
	{
		return ret;
	}

	auto const import_table_section_and_offset = convert_rva_to_disk_ptr(import_table_addr, hi);
	section_header const& it_section = *import_table_section_and_offset.first;
	std::uint32_t const it_disk = import_table_section_and_offset.second;
	VERIFY(it_section.m_raw_ptr + it_section.m_raw_size >= it_disk + import_table_size);
	std::uint32_t const max_it_size = import_table_size / sizeof(import_directory_entry);
	VERIFY(max_it_size <= 1024); // 1k DLLs should be enough for everybody.
	import_directory_entry const* const import_directory_table = reinterpret_cast<import_directory_entry const*>(file_data + it_disk);
	std::uint32_t it_size = 0xFFFFFFFF;
	for(std::uint32_t i = 0; i != max_it_size; ++i)
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
			it_size = i;
			break;
		}
	}
	VERIFY(it_size != 0xFFFFFFFF);
	ret.m_dlls.reserve(it_size);
	for(std::uint32_t i = 0; i != it_size; ++i)
	{
		std::uint32_t const name_rva = import_directory_table[i].m_name;
		auto const name_sct_dsk = convert_rva_to_disk_ptr(name_rva, hi);
		section_header const& name_sct = *name_sct_dsk.first;
		std::uint32_t const name_dsk = name_sct_dsk.second;
		std::uint32_t name_len = 0xFFFFFFFF;
		// 1k DLL name should be enough for everybody.
		for(std::uint32_t i = 0; i != 1024 && i != name_sct.m_raw_size - name_dsk; ++i)
		{
			if(reinterpret_cast<char const*>(file_data + name_dsk)[i] == '\0')
			{
				name_len = i;
				break;
			}
		}
		VERIFY(name_len != 0xFFFFFFFF && name_len > 0);
		char const* const dll = reinterpret_cast<char const*>(file_data + name_dsk);
		VERIFY(is_ascii(dll, static_cast<int>(name_len)));
		ret.m_dlls.push_back(dll);
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
