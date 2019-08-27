#pragma once


#include <cstdint>


enum class e_pe_parse_coff_optional_header_standard_32_64
{
	ok,
	coff_has_wrong_optional,
	file_too_small,
	file_not_coff_optional,
};


struct coff_optional_header_standard_32
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
static_assert(sizeof(coff_optional_header_standard_32) == 28, "");
static_assert(sizeof(coff_optional_header_standard_32) == 0x1c, "");

struct coff_optional_header_standard_64
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
static_assert(sizeof(coff_optional_header_standard_64) == 24, "");
static_assert(sizeof(coff_optional_header_standard_64) == 0x18, "");

struct coff_optional_header_standard_32_64
{
	union
	{
		coff_optional_header_standard_32 m_32;
		coff_optional_header_standard_64 m_64;
	};
};


static constexpr std::uint32_t const s_coff_optional_sig_32 = 0x010b;
static constexpr std::uint32_t const s_coff_optional_sig_64 = 0x020b;


e_pe_parse_coff_optional_header_standard_32_64 pe_parse_coff_optional_header_standard_32_64(void const* const& file_data, int const& file_size, coff_optional_header_standard_32_64 const*& header);
