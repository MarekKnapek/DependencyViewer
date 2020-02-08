#pragma once


#include <cstddef>
#include <cstdint>


enum class pe_e_parse_mz_header
{
	ok,
	file_too_small,
	file_not_mz,
};


struct pe_dos_header
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
static_assert(sizeof(pe_dos_header) == 62, "");
static_assert(sizeof(pe_dos_header) == 0x3e, "");


pe_e_parse_mz_header pe_parse_mz_header(std::byte const* const file_data, int const file_size, pe_dos_header const** const header_out);
