#pragma once


#include "coff.h"
#include "coff_optional_standard.h"
#include "coff_optional_windows.h"

#include <cstdint>


struct data_directory
{
	std::uint32_t m_va;
	std::uint32_t m_size;
};
static_assert(sizeof(data_directory) == 8, "");
static_assert(sizeof(data_directory) == 0x8, "");

struct coff_full_32
{
	coff_header m_coff;
	coff_optional_header_standard_32 m_standard;
	coff_optional_header_windows_32 m_windows;
	data_directory m_directories[16];
};

struct coff_full_64
{
	coff_header m_coff;
	coff_optional_header_standard_64 m_standard;
	coff_optional_header_windows_64 m_windows;
	data_directory m_directories[16];
};

struct coff_full_32_64
{
	union
	{
		coff_full_32 m_32;
		coff_full_64 m_64;
	};
};


enum class e_directory_table
{
	export_table,
	import_table,
	resource_table,
	exception_table,
	certificate_table,
	base_relocation_table,
	debug,
	architecture,
	global_ptr,
	tls_table,
	load_config_table,
	bound_import,
	iat,
	delay_import_descriptor,
	clr_runtime_header,
	reserved,
};


bool pe_parse_coff_full_32_64(void const* const& file_data, int const& file_size, coff_full_32_64 const*& header);
