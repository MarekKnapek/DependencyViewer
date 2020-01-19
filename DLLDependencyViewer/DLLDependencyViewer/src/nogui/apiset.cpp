#include "apiset.h"

#include "assert.h"

#include <cassert>
#include <cstdint>
#include <string>

#include "my_windows.h"

#pragma warning(push)
#pragma warning(disable:4201) // warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable:4324) // warning C4324: 'xxx': structure was padded due to alignment specifier
#pragma warning(disable:4471) // warning C4471: 'xxx': a forward declaration of an unscoped enumeration must have an underlying type
#include <phnt.h>
#pragma warning(pop)


struct apiset_value_entry_v2
{
	std::uint32_t m_name_offset;
	std::uint16_t m_name_len;
	std::uint16_t m_padding1;
	std::uint32_t m_value_offset;
	std::uint16_t m_value_len;
	std::uint16_t m_padding2;
};
static_assert(sizeof(apiset_value_entry_v2) == 16, "");
static_assert(sizeof(apiset_value_entry_v2) == 0x10, "");

struct apiset_value_array_v2
{
	std::uint32_t m_count;
};
static_assert(sizeof(apiset_value_array_v2) == 4, "");
static_assert(sizeof(apiset_value_array_v2) == 0x4, "");

struct apiset_namespace_entry_v2
{
	std::uint32_t m_name_offset;
	std::uint16_t m_name_len;
	std::uint16_t m_padding;
	std::uint32_t m_data_offset;
};
static_assert(sizeof(apiset_namespace_entry_v2) == 12, "");
static_assert(sizeof(apiset_namespace_entry_v2) == 0xc, "");

struct apiset_namespace_v2
{
	std::uint32_t m_version;
	std::uint32_t m_count;
};
static_assert(sizeof(apiset_namespace_v2) == 8, "");
static_assert(sizeof(apiset_namespace_v2) == 0x8, "");


struct apiset_value_entry_v4
{
	std::uint32_t m_flags;
	std::uint32_t m_name_offset;
	std::uint16_t m_name_len;
	std::uint16_t m_padding1;
	std::uint32_t m_value_offset;
	std::uint16_t m_value_len;
	std::uint16_t m_padding2;
};
static_assert(sizeof(apiset_value_entry_v4) == 20, "");
static_assert(sizeof(apiset_value_entry_v4) == 0x14, "");

struct apiset_value_array_v4
{
	std::uint32_t m_flags;
	std::uint32_t m_count;
};
static_assert(sizeof(apiset_value_array_v4) == 8, "");
static_assert(sizeof(apiset_value_array_v4) == 0x8, "");

struct apiset_namespace_entry_v4
{
	std::uint32_t m_flags;
	std::uint32_t m_name_offset;
	std::uint16_t m_name_len;
	std::uint16_t m_padding;
	std::uint32_t m_alias_offset;
	std::uint32_t m_alias_len;
	std::uint32_t m_data_offset;
};
static_assert(sizeof(apiset_namespace_entry_v4) == 24, "");
static_assert(sizeof(apiset_namespace_entry_v4) == 0x18, "");

struct apiset_namespace_v4
{
	std::uint32_t m_version;
	std::uint32_t m_size;
	std::uint32_t m_flags;
	std::uint32_t m_count;
};
static_assert(sizeof(apiset_namespace_v4) == 16, "");
static_assert(sizeof(apiset_namespace_v4) == 0x10, "");


typedef apiset_value_entry_v4 apiset_value_entry_v6;
static_assert(sizeof(apiset_value_entry_v6) == 20, "");
static_assert(sizeof(apiset_value_entry_v6) == 0x14, "");

struct apiset_namespace_entry_v6
{
	std::uint32_t m_flags;
	std::uint32_t m_name_offset;
	std::uint16_t m_name_len;
	std::uint16_t m_padding1;
	std::uint32_t m_name_len_for_hash;
	std::uint32_t m_data_offset;
	std::uint32_t m_count;
};
static_assert(sizeof(apiset_namespace_entry_v6) == 24, "");
static_assert(sizeof(apiset_namespace_entry_v6) == 0x18, "");

struct apiset_hash_entry_v6
{
	std::uint32_t m_hash;
	std::uint32_t m_index;
};
static_assert(sizeof(apiset_hash_entry_v6) == 8, "");
static_assert(sizeof(apiset_hash_entry_v6) == 0x8, "");

struct apiset_namespace_v6
{
	std::uint32_t m_version;
	std::uint32_t m_size;
	std::uint32_t m_flags;
	std::uint32_t m_count;
	std::uint32_t m_entries_offset;
	std::uint32_t m_hahses_offset;
	std::uint32_t m_multiplier;
};
static_assert(sizeof(apiset_namespace_v6) == 28, "");
static_assert(sizeof(apiset_namespace_v6) == 0x1c, "");


bool parse_apiset_v2(std::byte const* const data, std::uint32_t const data_len);
bool parse_apiset_v4(std::byte const* const data, std::uint32_t const data_len);
bool parse_apiset_v6(std::byte const* const data, std::uint32_t const data_len);


bool parse_apiset()
{
	auto const tib_offset = static_cast<unsigned long>(reinterpret_cast<std::uintptr_t>(&(static_cast<NT_TIB*>(nullptr)->Self)));
	#if defined _M_IX86
	static_assert(sizeof(decltype(__readfsdword(0))) == sizeof(void*), "");
	auto const tib_auto = __readfsdword(tib_offset);
	#elif defined _M_X64
	static_assert(sizeof(decltype(__readgsqword(0))) == sizeof(void*), "");
	auto const tib_auto = __readgsqword(tib_offset);
	#else
	#error Unknown architecture.
	#endif
	NT_TIB const* const tib = reinterpret_cast<NT_TIB const*>(tib_auto);
	TEB const* const teb = reinterpret_cast<TEB const*>(tib);
	PEB const* const peb = teb->ProcessEnvironmentBlock;
	std::byte const* const apiset_map = reinterpret_cast<std::byte const*>(peb->ApiSetMap);
	std::uint32_t const& apiset_map_ver = *reinterpret_cast<std::uint32_t const*>(apiset_map);
	switch(apiset_map_ver)
	{
		case 2:
		{
			MEMORY_BASIC_INFORMATION mbi;
			SIZE_T const written = VirtualQuery(static_cast<void const*>(apiset_map), &mbi, sizeof(mbi));
			assert(written != 0 && written == sizeof(mbi));
			assert(static_cast<std::byte const*>(mbi.BaseAddress) <= apiset_map);
			assert(mbi.AllocationBase <= mbi.BaseAddress);
			std::uint32_t const apiset_map_size = static_cast<std::uint32_t>(mbi.RegionSize - (apiset_map - static_cast<std::byte const*>(mbi.BaseAddress)));
			bool const parsed_v2 = parse_apiset_v2(apiset_map, apiset_map_size);
			WARN_M_R(parsed_v2, L"Failed to parse_apiset_v2.", false);
		}
		break;
		case 3: [[fallthrough]];
		case 4:
		{
			std::uint32_t const apiset_map_size = reinterpret_cast<apiset_namespace_v4 const*>(apiset_map)->m_size;
			bool const parsed_v4 = parse_apiset_v4(apiset_map, apiset_map_size);
			WARN_M_R(parsed_v4, L"Failed to parse_apiset_v2.", false);
		}
		break;
		case 5: [[fallthrough]];
		case 6:
		{
			std::uint32_t const apiset_map_size = reinterpret_cast<apiset_namespace_v6 const*>(apiset_map)->m_size;
			bool const parsed_v6 = parse_apiset_v6(apiset_map, apiset_map_size);
			WARN_M_R(parsed_v6, L"Failed to parse_apiset_v6.", false);
		}
		break;
		default:
		{
			WARN_M(false, L"Unknown apiset version.");
			return true;
		}
		break;
	}
	return true;
}


bool parse_apiset_v2(std::byte const* const data, std::uint32_t const data_len)
{
	std::uint32_t const namespace_raw = 0;
	WARN_M_R(namespace_raw < data_len, L"Out of bounds.", false);
	WARN_M_R(sizeof(apiset_namespace_v2) <= data_len - namespace_raw, L"Not enough room.", false);
	apiset_namespace_v2 const& namespc = *reinterpret_cast<apiset_namespace_v2 const*>(data + namespace_raw);
	assert(namespc.m_version == 2);

	std::uint32_t const namespc_entries_raw = namespace_raw + sizeof(apiset_namespace_v2);
	WARN_M_R(namespc_entries_raw < data_len, L"Out of bounds.", false);
	WARN_M_R(namespc.m_count * sizeof(apiset_namespace_entry_v2) <= data_len - namespc_entries_raw, L"Not enough room.", false);
	apiset_namespace_entry_v2 const* const namespc_entries = reinterpret_cast<apiset_namespace_entry_v2 const*>(data + namespc_entries_raw);

	for(std::uint32_t i = 0; i != namespc.m_count; ++i)
	{
		apiset_namespace_entry_v2 const& namespc_entry = namespc_entries[i];

		WARN_M_R(namespc_entry.m_name_offset < data_len, L"Out of bounds.", false);
		WARN_M_R(namespc_entry.m_name_len <= data_len - namespc_entry.m_name_offset, L"Not enough room.", false);
		WARN_M_R(namespc_entry.m_name_len % 2 == 0, L"Bad string length.", false);
		wchar_t const* const namespc_entry_name = reinterpret_cast<wchar_t const*>(data + namespc_entry.m_name_offset);
		std::wstring const namespc_entry_name_s{namespc_entry_name, namespc_entry_name + namespc_entry.m_name_len / 2};

		std::uint32_t const value_array_raw = namespc_entry.m_data_offset;
		WARN_M_R(value_array_raw < data_len, L"Out of bounds.", false);
		WARN_M_R(sizeof(apiset_value_array_v2) <= data_len - value_array_raw, L"Not enough room.", false);
		apiset_value_array_v2 const& value_array = *reinterpret_cast<apiset_value_array_v2 const*>(data + value_array_raw);

		std::uint32_t const value_entries_raw = value_array_raw + sizeof(apiset_value_array_v2);
		WARN_M_R(value_entries_raw < data_len, L"Out of bounds.", false);
		WARN_M_R(value_array.m_count * sizeof(apiset_value_entry_v2) <= data_len - value_entries_raw, L"Not enough room.", false);
		apiset_value_entry_v2 const* const value_entries = reinterpret_cast<apiset_value_entry_v2 const*>(data + value_entries_raw);

		for(std::uint32_t j = 0; j != value_array.m_count; ++j)
		{
			apiset_value_entry_v2 const& value_entry = value_entries[j];

			WARN_M_R(value_entry.m_name_offset < data_len, L"Out of bounds.", false);
			WARN_M_R(value_entry.m_name_len <= data_len - value_entry.m_name_offset, L"Not enough room.", false);
			WARN_M_R(value_entry.m_name_len % 2 == 0, L"Bad string length.", false);
			wchar_t const* const value_entry_name = reinterpret_cast<wchar_t const*>(data + value_entry.m_name_offset);
			std::wstring const value_entry_name_s{value_entry_name, value_entry_name + value_entry.m_name_len / 2};

			WARN_M_R(value_entry.m_value_offset < data_len, L"Out of bounds.", false);
			WARN_M_R(value_entry.m_value_len <= data_len - value_entry.m_value_offset, L"Not enough room.", false);
			WARN_M_R(value_entry.m_value_len % 2 == 0, L"Bad string length.", false);
			wchar_t const* const value_entry_value = reinterpret_cast<wchar_t const*>(data + value_entry.m_value_offset);
			std::wstring const value_entry_value_s{value_entry_value, value_entry_value + value_entry.m_value_len / 2};
		}
	}

	return true;
}

bool parse_apiset_v4(std::byte const* const data, std::uint32_t const data_len)
{
	std::uint32_t const namespace_raw = 0;
	WARN_M_R(namespace_raw < data_len, L"Out of bounds.", false);
	WARN_M_R(sizeof(apiset_namespace_v4) <= data_len - namespace_raw, L"Not enough room.", false);
	apiset_namespace_v4 const& namespc = *reinterpret_cast<apiset_namespace_v4 const*>(data + namespace_raw);
	assert(namespc.m_version == 3 || namespc.m_version == 4);

	std::uint32_t const namespc_entries_raw = namespace_raw + sizeof(apiset_namespace_v4);
	WARN_M_R(namespc_entries_raw < data_len, L"Out of bounds.", false);
	WARN_M_R(namespc.m_count * sizeof(apiset_namespace_entry_v4) <= data_len - namespc_entries_raw, L"Not enough room.", false);
	apiset_namespace_entry_v4 const* const namespc_entries = reinterpret_cast<apiset_namespace_entry_v4 const*>(data + namespc_entries_raw);

	for(std::uint32_t i = 0; i != namespc.m_count; ++i)
	{
		apiset_namespace_entry_v4 const& namespc_entry = namespc_entries[i];

		WARN_M_R(namespc_entry.m_name_offset < data_len, L"Out of bounds.", false);
		WARN_M_R(namespc_entry.m_name_len <= data_len - namespc_entry.m_name_offset, L"Not enough room.", false);
		WARN_M_R(namespc_entry.m_name_len % 2 == 0, L"Bad string length.", false);
		wchar_t const* const namespc_entry_name = reinterpret_cast<wchar_t const*>(data + namespc_entry.m_name_offset);
		std::wstring const namespc_entry_name_s{namespc_entry_name, namespc_entry_name + namespc_entry.m_name_len / 2};

		std::uint32_t const value_array_raw = namespc_entry.m_data_offset;
		WARN_M_R(value_array_raw < data_len, L"Out of bounds.", false);
		WARN_M_R(sizeof(apiset_value_array_v4) <= data_len - value_array_raw, L"Not enough room.", false);
		apiset_value_array_v4 const& value_array = *reinterpret_cast<apiset_value_array_v4 const*>(data + value_array_raw);

		std::uint32_t const value_entries_raw = value_array_raw + sizeof(apiset_value_array_v4);
		WARN_M_R(value_entries_raw < data_len, L"Out of bounds.", false);
		WARN_M_R(value_array.m_count * sizeof(apiset_value_entry_v4) <= data_len - value_entries_raw, L"Not enough room.", false);
		apiset_value_entry_v4 const* const value_entries = reinterpret_cast<apiset_value_entry_v4 const*>(data + value_entries_raw);

		for(std::uint32_t j = 0; j != value_array.m_count; ++j)
		{
			apiset_value_entry_v4 const& value_entry = value_entries[j];

			WARN_M_R(value_entry.m_name_offset < data_len, L"Out of bounds.", false);
			WARN_M_R(value_entry.m_name_len <= data_len - value_entry.m_name_offset, L"Not enough room.", false);
			WARN_M_R(value_entry.m_name_len % 2 == 0, L"Bad string length.", false);
			wchar_t const* const value_entry_name = reinterpret_cast<wchar_t const*>(data + value_entry.m_name_offset);
			std::wstring const value_entry_name_s{value_entry_name, value_entry_name + value_entry.m_name_len / 2};

			WARN_M_R(value_entry.m_value_offset < data_len, L"Out of bounds.", false);
			WARN_M_R(value_entry.m_value_len <= data_len - value_entry.m_value_offset, L"Not enough room.", false);
			WARN_M_R(value_entry.m_value_len % 2 == 0, L"Bad string length.", false);
			wchar_t const* const value_entry_value = reinterpret_cast<wchar_t const*>(data + value_entry.m_value_offset);
			std::wstring const value_entry_value_s{value_entry_value, value_entry_value + value_entry.m_value_len / 2};
		}
	}

	return true;
}

bool parse_apiset_v6(std::byte const* const data, std::uint32_t const data_len)
{
	std::uint32_t const namespace_raw = 0;
	WARN_M_R(namespace_raw < data_len, L"Out of bounds.", false);
	WARN_M_R(sizeof(apiset_namespace_v6) <= data_len - namespace_raw, L"Not enough room.", false);
	apiset_namespace_v6 const& namespc = *reinterpret_cast<apiset_namespace_v6 const*>(data + namespace_raw);
	assert(namespc.m_version == 5 || namespc.m_version == 6);

	std::uint32_t const namespc_entries_raw = namespc.m_entries_offset;
	WARN_M_R(namespc_entries_raw < data_len, L"Out of bounds.", false);
	WARN_M_R(namespc.m_count * sizeof(apiset_namespace_entry_v6) <= data_len - namespc_entries_raw, L"Not enough room.", false);
	apiset_namespace_entry_v6 const* const namespc_entries = reinterpret_cast<apiset_namespace_entry_v6 const*>(data + namespc_entries_raw);

	for(std::uint32_t i = 0; i != namespc.m_count; ++i)
	{
		apiset_namespace_entry_v6 const& namespc_entry = namespc_entries[i];

		WARN_M_R(namespc_entry.m_name_offset < data_len, L"Out of bounds.", false);
		WARN_M_R(namespc_entry.m_name_len <= data_len - namespc_entry.m_name_offset, L"Not enough room.", false);
		WARN_M_R(namespc_entry.m_name_len % 2 == 0, L"Bad string length.", false);
		wchar_t const* const namespc_entry_name = reinterpret_cast<wchar_t const*>(data + namespc_entry.m_name_offset);
		std::wstring const namespc_entry_name_s{namespc_entry_name, namespc_entry_name + namespc_entry.m_name_len / 2};

		std::uint32_t const value_entries_raw = namespc_entry.m_data_offset;
		WARN_M_R(value_entries_raw < data_len, L"Out of bounds.", false);
		WARN_M_R(namespc_entry.m_count * sizeof(apiset_value_entry_v6) <= data_len - value_entries_raw, L"Not enough room.", false);
		apiset_value_entry_v6 const* const value_entries = reinterpret_cast<apiset_value_entry_v6 const*>(data + value_entries_raw);

		for(std::uint32_t j = 0; j != namespc_entry.m_count; ++j)
		{
			apiset_value_entry_v6 const& value_entry = value_entries[j];

			WARN_M_R(value_entry.m_name_offset < data_len, L"Out of bounds.", false);
			WARN_M_R(value_entry.m_name_len <= data_len - value_entry.m_name_offset, L"Not enough room.", false);
			WARN_M_R(value_entry.m_name_len % 2 == 0, L"Bad string length.", false);
			wchar_t const* const value_entry_name = reinterpret_cast<wchar_t const*>(data + value_entry.m_name_offset);
			std::wstring const value_entry_name_s{value_entry_name, value_entry_name + value_entry.m_name_len / 2};

			WARN_M_R(value_entry.m_value_offset < data_len, L"Out of bounds.", false);
			WARN_M_R(value_entry.m_value_len <= data_len - value_entry.m_value_offset, L"Not enough room.", false);
			WARN_M_R(value_entry.m_value_len % 2 == 0, L"Bad string length.", false);
			wchar_t const* const value_entry_value = reinterpret_cast<wchar_t const*>(data + value_entry.m_value_offset);
			std::wstring const value_entry_value_s{value_entry_value, value_entry_value + value_entry.m_value_len / 2};
		}
	}

	return true;
}
