#include "dependency_locator.h"

#include "known_dlls.h"
#include "unicode.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>

#include "my_windows.h"


bool locate_dependency(dependency_locator& self)
{
	if(locate_dependency_sxs(self)) return true;
	if(locate_dependency_known_dlls(self)) return true;
	if(locate_dependency_application_dir(self)) return true;
	if(locate_dependency_system32(self)) return true;
	if(locate_dependency_system16(self)) return true;
	if(locate_dependency_windows(self)) return true;
	if(locate_dependency_current_dir(self)) return true;
	if(locate_dependency_environment_path(self)) return true;
	return false;
}


bool locate_dependency_sxs(dependency_locator&)
{
	return false;
}

bool locate_dependency_known_dlls(dependency_locator& self)
{
	string_handle const& dependency = *self.m_dependency;
	std::string& tmpn = self.m_tmpn;
	self.m_tmpn.resize(size(dependency));
	std::transform(begin(dependency), end(dependency), begin(tmpn), [](auto const& e){ return to_lowercase(e); });
	string const dll_name_s{tmpn.c_str(), static_cast<int>(tmpn.size())};
	string_handle const dll_name_sh{&dll_name_s};
	std::vector<std::string> const& known_dll_names = get_known_dll_names_sorted_lowercase_ascii();
	auto const it = std::lower_bound(known_dll_names.begin(), known_dll_names.end(), dll_name_sh, [](auto const& e, auto const& v){ string const e_s{e.c_str(), static_cast<int>(e.size())}; return string_handle{&e_s} < v; });
	if(it == known_dll_names.end())
	{
		return false;
	}
	string const s{it->c_str(), static_cast<int>(it->size())};
	if(string_handle{&s} != dll_name_sh)
	{
		return false;
	}
	self.m_result = std::filesystem::path{get_knonw_dlls_path()}.append(*it);
	return true;
}

bool locate_dependency_application_dir(dependency_locator& self)
{
	wstring_handle const& main_path = *self.m_main_path;
	string_handle const& dependency = *self.m_dependency;
	auto const p = std::filesystem::path{begin(main_path), end(main_path)}.replace_filename({begin(dependency), end(dependency)});
	if(!std::filesystem::exists(p))
	{
		return false;
	}
	self.m_result = p;
	return true;
}

bool locate_dependency_system32(dependency_locator& self)
{
	string_handle const& dependency = *self.m_dependency;
	std::array<wchar_t, 32 * 1024> buff;
	UINT const got_sys = GetSystemDirectoryW(buff.data(), static_cast<UINT>(buff.size()));
	assert(got_sys != 0);
	assert(got_sys < static_cast<UINT>(buff.size()));
	auto const p = std::filesystem::path{buff.data(), buff.data() + got_sys}.append(begin(dependency), end(dependency));
	if(!std::filesystem::exists(p))
	{
		return false;
	}
	self.m_result = p;
	return true;
}

bool locate_dependency_system16(dependency_locator&)
{
	// The 16-bit system directory. There is no function that obtains the path of this directory, but it is searched.
	return false;
}

bool locate_dependency_windows(dependency_locator& self)
{
	string_handle const& dependency = *self.m_dependency;
	std::array<wchar_t, 32 * 1024> buff;
	UINT const got_win = GetWindowsDirectoryW(buff.data(), static_cast<UINT>(buff.size()));
	assert(got_win != 0);
	assert(got_win < static_cast<UINT>(buff.size()));
	auto const p = std::filesystem::path{buff.data(), buff.data() + got_win}.append(begin(dependency), end(dependency));
	if(!std::filesystem::exists(p))
	{
		return false;
	}
	self.m_result = p;
	return true;
}

bool locate_dependency_current_dir(dependency_locator& self)
{
	string_handle const& dependency = *self.m_dependency;
	std::array<wchar_t, 32 * 1024> buff;
	DWORD const got_currdir = GetCurrentDirectoryW(static_cast<DWORD>(buff.size()), buff.data());
	assert(got_currdir != 0);
	assert(got_currdir < static_cast<DWORD>(buff.size()));
	auto const p = std::filesystem::path{buff.data(), buff.data() + got_currdir}.append(begin(dependency), end(dependency));
	if(!std::filesystem::exists(p))
	{
		return false;
	}
	self.m_result = p;
	return true;
}

bool locate_dependency_environment_path(dependency_locator& self)
{
	string_handle const& dependency = *self.m_dependency;
	std::array<wchar_t, 32 * 1024> buff;
	DWORD const got_env = GetEnvironmentVariableW(L"PATH", buff.data(), static_cast<DWORD>(buff.size()));
	assert(got_env != 0);
	assert(got_env < static_cast<DWORD>(buff.size()));
	auto const buff_end = buff.begin() + got_env;
	std::filesystem::path p;
	auto start = buff.begin();
	for(;;)
	{
		auto const it = std::find(start, buff_end, L';');
		p.assign(start, it).append(begin(dependency), end(dependency));
		if(std::filesystem::exists(p))
		{
			self.m_result = p;
			return true;
		}
		if(it == buff_end)
		{
			break;
		}
		start = it + 1;
	}
	return false;
}
