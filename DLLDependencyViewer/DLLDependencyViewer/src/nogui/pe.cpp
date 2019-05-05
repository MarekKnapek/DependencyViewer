#include "pe.h"

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


static constexpr char const s_bad_format[] = "Bad format.";


#define VERIFY(X) do{ assert(X); if(!(X)){ throw s_bad_format; } }while(false)


void const* pe_get_coff_header(void const* const fd, int const fs)
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

	return file_data + dos_hdr.m_pe_offset;
}
