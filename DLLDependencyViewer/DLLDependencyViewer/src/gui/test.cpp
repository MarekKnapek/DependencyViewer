#include "test.h"

#include "../nogui/memory_manager.h"
#include "../nogui/memory_mapped_file.h"
#include "../nogui/pe.h"

#include <cassert>
#include <cwchar>
#include <experimental/filesystem>
#include <iterator>
#include <memory>

#include <windows.h>


static constexpr wchar_t const s_cmd_arg_test[] = L"/test";


namespace fs = std::experimental::filesystem;


struct local_free_deleter
{
	void operator()(HLOCAL const& ptr) const;
};


void test()
{
	wchar_t const* const cmd_line = GetCommandLineW();
	int argc;
	wchar_t** const argv = CommandLineToArgvW(cmd_line, &argc);
	std::unique_ptr<void, local_free_deleter> sp_argv(reinterpret_cast<void*>(argv));
	if(argc != 3)
	{
		return;
	}
	if(std::wcsncmp(argv[1], s_cmd_arg_test, std::size(s_cmd_arg_test) - 1) != 0)
	{
		return;
	}
	fs::recursive_directory_iterator it(argv[2]);
	for(auto const& e : it)
	{
		auto const& p = e.path();
		if(fs::is_directory(p))
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
			if(file_size.LowPart >= 2'147'483'647)
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
		if(static_cast<char const*>(mmf.begin())[0] != 'M')
		{
			continue;
		}
		if(static_cast<char const*>(mmf.begin())[1] != 'Z')
		{
			continue;
		}
		if(len < 128)
		{
			continue;
		}
		std::uint16_t const& new_header_offset = *reinterpret_cast<std::uint16_t const*>(reinterpret_cast<char const*>(mmf.begin()) + 60);
		if(len < new_header_offset + 4)
		{
			continue;
		}
		std::uint32_t const& new_header_header = *reinterpret_cast<std::uint32_t const*>(reinterpret_cast<char const*>(mmf.begin()) + new_header_offset);
		if(new_header_header != 0x00004550)
		{
			continue;
		}
		memory_manager mm;
		pe_header_info hi;
		pe_import_table_info it;
		pe_export_table_info et;
		pe_resources_table_info rs;
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
		try
		{
			it = pe_process_import_table(mmf.begin(), mmf.size(), hi, mm);
		}
		catch(wchar_t const* const&)
		{
			OutputDebugStringW(p.c_str());
			OutputDebugStringW(L"\n");
		}
		try
		{
			et = pe_process_export_table(mmf.begin(), mmf.size(), hi, mm);
		}
		catch(wchar_t const* const&)
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


void local_free_deleter::operator()(HLOCAL const& ptr) const
{
	HLOCAL const freed = LocalFree(ptr);
	assert(freed == nullptr);
}
