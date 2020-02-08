#pragma once


#include <cstddef>
#include <cstdint>


enum class pe_e_parse_coff_optional_header_standard_32_64
{
	ok,
	coff_has_wrong_optional,
	file_too_small,
	file_not_coff_optional,
};


struct pe_coff_optional_header_standard_32
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
static_assert(sizeof(pe_coff_optional_header_standard_32) == 28, "");
static_assert(sizeof(pe_coff_optional_header_standard_32) == 0x1c, "");

struct pe_coff_optional_header_standard_64
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
static_assert(sizeof(pe_coff_optional_header_standard_64) == 24, "");
static_assert(sizeof(pe_coff_optional_header_standard_64) == 0x18, "");

struct pe_coff_optional_header_standard_32_64
{
	union
	{
		pe_coff_optional_header_standard_32 m_32;
		pe_coff_optional_header_standard_64 m_64;
	};
};


pe_e_parse_coff_optional_header_standard_32_64 pe_parse_coff_optional_header_standard_32_64(std::byte const* const file_data, int const file_size, pe_coff_optional_header_standard_32_64 const** const header_out);
bool pe_is_32_bit(pe_coff_optional_header_standard_32 const& header);
