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
		auto const& len = fs::file_size(p);
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
		auto const& hi = pe_process_header(mmf.begin(), mmf.size());
		auto const& it = pe_process_import_table(mmf.begin(), mmf.size(), hi, mm);
		auto const& et = pe_process_export_table(mmf.begin(), mmf.size(), hi, mm);
		auto const& rs = pe_process_resource_table(mmf.begin(), mmf.size(), hi, mm);
	}
}


void local_free_deleter::operator()(HLOCAL const& ptr) const
{
	HLOCAL const freed = LocalFree(ptr);
	assert(freed == nullptr);
}
