#include "test.h"

#include "../nogui/memory_manager.h"
#include "../nogui/memory_mapped_file.h"
#include "../nogui/pe.h"
#include "../nogui/pe2.h"
#include "../nogui/smart_handle.h"
#include "../nogui/smart_local_free.h"

#include <cassert>
#include <cwchar>
#include <filesystem>
#include <iterator>
#include <memory>

#include "../nogui/my_windows.h"
#include <shellapi.h>


#define s_very_big_int (2'147'483'647)


void test()
{
	wchar_t const* const cmd_line = GetCommandLineW();
	int argc;
	wchar_t** const argv = CommandLineToArgvW(cmd_line, &argc);
	smart_local_free const sp_argv(reinterpret_cast<void*>(argv));
	if(argc != 3)
	{
		return;
	}
	if(std::wcsncmp(argv[1], s_cmd_arg_test, std::size(s_cmd_arg_test) - 1) != 0)
	{
		return;
	}
	std::filesystem::recursive_directory_iterator dir_it(argv[2], std::filesystem::directory_options::skip_permission_denied);
	for(auto const& e : dir_it)
	{
		auto const& p = e.path();
		if(std::filesystem::is_directory(p))
		{
			continue;
		}
		int len;
		{
			HANDLE const file = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if(file == INVALID_HANDLE_VALUE)
			{
				continue;
			}
			smart_handle sp_file(file);
			LARGE_INTEGER file_size;
			BOOL const got_size = GetFileSizeEx(file, &file_size);
			if(got_size == 0)
			{
				continue;
			}
			if(file_size.HighPart != 0)
			{
				continue;
			}
			if(file_size.LowPart >= s_very_big_int)
			{
				continue;
			}
			len = static_cast<int>(file_size.LowPart);
		}
		if(len < 2)
		{
			continue;
		}
		memory_mapped_file mmf;
		try
		{
			mmf = memory_mapped_file(p.c_str());
		}
		catch(wchar_t const*)
		{
			continue;
		}
		if(reinterpret_cast<char const*>(mmf.begin())[0] != 'M')
		{
			continue;
		}
		if(reinterpret_cast<char const*>(mmf.begin())[1] != 'Z')
		{
			continue;
		}
		if(len < 128)
		{
			continue;
		}
		std::uint16_t const& new_header_offset = *reinterpret_cast<std::uint16_t const*>(mmf.begin() + 60);
		if(len < new_header_offset + 4)
		{
			continue;
		}
		std::uint32_t const& new_header_header = *reinterpret_cast<std::uint32_t const*>(mmf.begin() + new_header_offset);
		if(new_header_header != 0x00004550)
		{
			continue;
		}
		memory_manager mm;
		pe_header_info hi;
		pe_resources_table_info rs;
		std::uint16_t enpt_count;
		std::uint16_t const* enpt;
		allocator enpt_alloc;
		try
		{
			hi = pe_process_header(mmf.begin(), mmf.size());
		}
		catch(wchar_t const* const&)
		{
			OutputDebugStringW(p.c_str());
			OutputDebugStringW(L"\n");
			continue;
		}
		pe_import_table_info iti;
		pe_export_table_info eti;
		pe_tables tables;
		tables.m_tmp_alc = &enpt_alloc;
		tables.m_iti_out = &iti;
		tables.m_eti_out = &eti;
		tables.m_enpt_count_out = &enpt_count;
		tables.m_enpt_out = &enpt;
		bool const tables_processed = pe_process_all(mmf.begin(), mmf.size(), mm, &tables);
		if(!tables_processed)
		{
			OutputDebugStringW(p.c_str());
			OutputDebugStringW(L"\n");
		}
		try
		{
			rs = pe_process_resource_table(mmf.begin(), mmf.size(), hi, mm);
		}
		catch(wchar_t const* const&)
		{
			OutputDebugStringW(p.c_str());
			OutputDebugStringW(L"\n");
		}
	}
}
