#include "search.h"

#include <array>
#include <experimental/filesystem>

#include <windows.h>


namespace fs = std::experimental::filesystem;


std::wstring search(searcher& sch, std::wstring const& dll_name)
{
	// Standard Search Order for Desktop Applications
	// - SafeDllSearchMode is enabled
	// -- The directory from which the application loaded.
	// -- The system directory. Use the GetSystemDirectory function to get the path of this directory.
	// -- The 16-bit system directory. There is no function that obtains the path of this directory, but it is searched.
	// -- The Windows directory. Use the GetWindowsDirectory function to get the path of this directory.
	// -- The current directory.
	// -- The directories that are listed in the PATH environment variable. Note that this does not include the per-application path specified by the App Paths registry key. The App Paths key is not used when computing the DLL search path.

	fs::path p = *sch.m_main_file_path;
	p.replace_filename(dll_name);
	if(fs::exists(p))
	{
		return p;
	}

	std::array<wchar_t, 32 * 1024> buff;
	UINT const got_sys = GetSystemDirectoryW(buff.data(), static_cast<UINT>(buff.size()));
	p.assign(buff.data(), buff.data() + got_sys);
	p.append(dll_name);
	if(fs::exists(p))
	{
		return p;
	}

	// TODO: 16 bit system.

	UINT const got_win = GetWindowsDirectoryW(buff.data(), static_cast<UINT>(buff.size()));
	p.assign(buff.data(), buff.data() + got_win);
	p.append(dll_name);
	if(fs::exists(p))
	{
		return p;
	}

	// TODO: Current directory.

	DWORD const got_env = GetEnvironmentVariableW(L"PATH", buff.data(), static_cast<DWORD>(buff.size()));
	if(got_env != 0)
	{
		std::wstring const path_env(buff.data(), buff.data() + got_env);
		std::size_t last = 0;
		for(;;)
		{
			std::size_t const idx = path_env.find_first_of(L';', last);
			if(idx == std::wstring::npos)
			{
				p.assign(cbegin(path_env) + last, cend(path_env));
				p.append(dll_name);
				if(fs::exists(p))
				{
					return p;
				}
				break;
			}
			else
			{
				p.assign(cbegin(path_env) + last, cbegin(path_env) + idx);
				p.append(dll_name);
				if(fs::exists(p))
				{
					return p;
				}
				last = idx + 1;
			}
		}
	}

	return {};
}
