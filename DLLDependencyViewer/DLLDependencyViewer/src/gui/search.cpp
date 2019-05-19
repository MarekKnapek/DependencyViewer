#include "search.h"

#include "processor.h"

#include "../nogui/unique_strings.h"

#include <array>
#include <experimental/filesystem>

#include <windows.h>


namespace fs = std::experimental::filesystem;


wstring const* search(searcher& sch, string const* const& dll_name)
{
	// Standard Search Order for Desktop Applications
	// - SafeDllSearchMode is enabled
	// -- The directory from which the application loaded.
	// -- The system directory. Use the GetSystemDirectory function to get the path of this directory.
	// -- The 16-bit system directory. There is no function that obtains the path of this directory, but it is searched.
	// -- The Windows directory. Use the GetWindowsDirectory function to get the path of this directory.
	// -- The current directory.
	// -- The directories that are listed in the PATH environment variable. Note that this does not include the per-application path specified by the App Paths registry key. The App Paths key is not used when computing the DLL search path.

	std::string& tmpn = sch.m_mo->m_tmpn;
	std::wstring& tmpw = sch.m_mo->m_tmpw;
	fs::path& tmpp = sch.m_mo->m_tmpp;

	tmpp = *sch.m_main_file_path;
	tmpw.resize(dll_name->m_len);
	std::transform(dll_name->m_str, dll_name->m_str + dll_name->m_len, tmpw.begin(), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
	tmpp.replace_filename(tmpw);
	if(fs::exists(tmpp))
	{
		tmpw = tmpp.wstring();
		return sch.m_mo->m_mm.m_wstrs.add_string(tmpw.c_str(), static_cast<int>(tmpw.size()), sch.m_mo->m_mm.m_alc);
	}

	std::array<wchar_t, 32 * 1024> buff;
	UINT const got_sys = GetSystemDirectoryW(buff.data(), static_cast<UINT>(buff.size()));
	tmpp.assign(buff.data(), buff.data() + got_sys);
	tmpp.append(tmpw);
	if(fs::exists(tmpp))
	{
		tmpw = tmpp.wstring();
		return sch.m_mo->m_mm.m_wstrs.add_string(tmpw.c_str(), static_cast<int>(tmpw.size()), sch.m_mo->m_mm.m_alc);
	}

	// TODO: 16 bit system.

	UINT const got_win = GetWindowsDirectoryW(buff.data(), static_cast<UINT>(buff.size()));
	tmpp.assign(buff.data(), buff.data() + got_win);
	tmpp.append(tmpw);
	if(fs::exists(tmpp))
	{
		tmpw = tmpp.wstring();
		return sch.m_mo->m_mm.m_wstrs.add_string(tmpw.c_str(), static_cast<int>(tmpw.size()), sch.m_mo->m_mm.m_alc);
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
				tmpp.assign(cbegin(path_env) + last, cend(path_env));
				tmpp.append(tmpw);
				if(fs::exists(tmpp))
				{
					tmpw = tmpp.wstring();
					return sch.m_mo->m_mm.m_wstrs.add_string(tmpw.c_str(), static_cast<int>(tmpw.size()), sch.m_mo->m_mm.m_alc);
				}
				break;
			}
			else
			{
				tmpp.assign(cbegin(path_env) + last, cbegin(path_env) + idx);
				tmpp.append(tmpw);
				if(fs::exists(tmpp))
				{
					tmpw = tmpp.wstring();
					return sch.m_mo->m_mm.m_wstrs.add_string(tmpw.c_str(), static_cast<int>(tmpw.size()), sch.m_mo->m_mm.m_alc);
				}
				last = idx + 1;
			}
		}
	}

	return nullptr;
}
