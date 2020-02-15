#include "test.h"

#include "../nogui/cassert_my.h"
#include "../nogui/memory_manager.h"
#include "../nogui/memory_mapped_file.h"
#include "../nogui/pe.h"
#include "../nogui/pe2.h"
#include "../nogui/scope_exit.h"
#include "../nogui/smart_handle.h"

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
	auto const fn_free_argv = mk::make_scope_exit([&](){ [[maybe_unused]] HLOCAL const freed = LocalFree(argv); assert(freed == nullptr); });
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
		memory_mapped_file const mmf(p.c_str());
		if(mmf.begin() == nullptr)
		{
			continue;
		}
		if(mmf.size() < 2)
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
		if(mmf.size() < 128)
		{
			continue;
		}
		std::uint16_t const& new_header_offset = *reinterpret_cast<std::uint16_t const*>(mmf.begin() + 60);
		if(mmf.size() < new_header_offset + 4)
		{
			continue;
		}
		std::uint32_t const& new_header_header = *reinterpret_cast<std::uint32_t const*>(mmf.begin() + new_header_offset);
		if(new_header_header != 0x00004550)
		{
			continue;
		}
		memory_manager mm;
		std::uint16_t enpt_count;
		std::uint16_t const* enpt;
		allocator enpt_alloc;
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
	}
}
