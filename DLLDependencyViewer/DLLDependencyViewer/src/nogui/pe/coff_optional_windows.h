#pragma once


#include <cstddef>
#include <cstdint>


enum class pe_e_parse_coff_optional_header_windows_32_64
{
	ok,
	coff_has_wrong_optional,
	file_too_small,
};


struct pe_coff_optional_header_windows_32
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
static_assert(sizeof(pe_coff_optional_header_windows_32) == 68, "");
static_assert(sizeof(pe_coff_optional_header_windows_32) == 0x44, "");

struct pe_coff_optional_header_windows_64
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
static_assert(sizeof(pe_coff_optional_header_windows_64) == 88, "");
static_assert(sizeof(pe_coff_optional_header_windows_64) == 0x58, "");

struct pe_coff_optional_header_windows_32_64
{
	union
	{
		pe_coff_optional_header_windows_32 m_32;
		pe_coff_optional_header_windows_64 m_64;
	};
};


pe_e_parse_coff_optional_header_windows_32_64 pe_parse_coff_optional_header_windows_32_64(std::byte const* const file_data, int const file_size, pe_coff_optional_header_windows_32_64 const** const header_out);
