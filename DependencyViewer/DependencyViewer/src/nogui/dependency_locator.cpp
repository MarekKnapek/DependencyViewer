#include "dependency_locator.h"

#include "assert_my.h"
#include "cassert_my.h"
#include "known_dlls.h"
#include "unicode.h"

#include <algorithm>
#include <array>

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


bool locate_dependency_sxs(dependency_locator& self)
{
	string_handle const& dependency = *self.m_dependency;
	ACTCTX_SECTION_KEYED_DATA actctx_section_keyed_data{};
	actctx_section_keyed_data.cbSize = sizeof(actctx_section_keyed_data);
	BOOL const actctx_data_found = FindActCtxSectionStringA(0, nullptr, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, dependency.m_string->m_str, &actctx_section_keyed_data);
	if(actctx_data_found == FALSE)
	{
		return false;
	}
	std::array<wchar_t, 1 * 1024> buff;
	WARN_M_R(dependency.m_string->m_len < static_cast<int>(buff.size()), L"File name too long.", false);
	std::transform(begin(dependency), end(dependency), buff.begin(), [](char const& ch) -> wchar_t { return static_cast<wchar_t>(ch); });
	buff[size(dependency)] = L'\0';
	self.m_result.resize(32 * 1024);
	DWORD const found = SearchPathW(nullptr, buff.data(), nullptr, static_cast<int>(self.m_result.size()), self.m_result.data(), nullptr);
	if(found == 0)
	{
		return false;
	}
	WARN_M_R(found < self.m_result.size(), L"Path too long.", false);
	self.m_result.resize(found);
	return true;
}

bool locate_dependency_known_dlls(dependency_locator& self)
{
	string_handle const& dependency = *self.m_dependency;
	std::string& tmpn = self.m_tmpn;
	self.m_tmpn.resize(size(dependency));
	std::transform(begin(dependency), end(dependency), begin(tmpn), [](auto const& e){ return to_lowercase(e); });
	string const dll_name_s{tmpn.c_str(), static_cast<int>(tmpn.size())};
	auto const& known_dll_names = known_dlls::get_names_sorted_lowercase_ascii();
	auto const it = std::lower_bound(known_dll_names.begin(), known_dll_names.end(), dll_name_s, [](auto const& e, auto const& v){ string const e_s{e.c_str(), static_cast<int>(e.size())}; return e_s < v; });
	if(it == known_dll_names.end())
	{
		return false;
	}
	string const s{it->c_str(), static_cast<int>(it->size())};
	if(s != dll_name_s)
	{
		return false;
	}
	self.m_result = std::filesystem::path{known_dlls::get_path()}.append(*it);
	return true;
}

bool locate_dependency_application_dir(dependency_locator& self)
{
	wstring_handle const& main_path = self.m_main_path;
	string_handle const& dependency = *self.m_dependency;
	std::filesystem::path& tmp_path = self.m_tmp_path;
	tmp_path.assign(begin(main_path), end(main_path)).replace_filename({begin(dependency), end(dependency)});
	if(!std::filesystem::exists(tmp_path))
	{
		return false;
	}
	self.m_result = tmp_path;
	return true;
}

bool locate_dependency_system32(dependency_locator& self)
{
	string_handle const& dependency = *self.m_dependency;
	std::filesystem::path& tmp_path = self.m_tmp_path;
	std::array<wchar_t, 32 * 1024> buff;
	UINT const got_sys = GetSystemDirectoryW(buff.data(), static_cast<UINT>(buff.size()));
	assert(got_sys != 0);
	assert(got_sys < static_cast<UINT>(buff.size()));
	tmp_path.assign(buff.data(), buff.data() + got_sys).append(begin(dependency), end(dependency));
	if(!std::filesystem::exists(tmp_path))
	{
		return false;
	}
	self.m_result = tmp_path;
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
	std::filesystem::path& tmp_path = self.m_tmp_path;
	std::array<wchar_t, 32 * 1024> buff;
	UINT const got_win = GetWindowsDirectoryW(buff.data(), static_cast<UINT>(buff.size()));
	assert(got_win != 0);
	assert(got_win < static_cast<UINT>(buff.size()));
	tmp_path.assign(buff.data(), buff.data() + got_win).append(begin(dependency), end(dependency));
	if(!std::filesystem::exists(tmp_path))
	{
		return false;
	}
	self.m_result = tmp_path;
	return true;
}

bool locate_dependency_current_dir(dependency_locator& self)
{
	string_handle const& dependency = *self.m_dependency;
	std::filesystem::path& tmp_path = self.m_tmp_path;
	std::array<wchar_t, 32 * 1024> buff;
	DWORD const got_currdir = GetCurrentDirectoryW(static_cast<DWORD>(buff.size()), buff.data());
	assert(got_currdir != 0);
	assert(got_currdir < static_cast<DWORD>(buff.size()));
	tmp_path.assign(buff.data(), buff.data() + got_currdir).append(begin(dependency), end(dependency));
	if(!std::filesystem::exists(tmp_path))
	{
		return false;
	}
	self.m_result = tmp_path;
	return true;
}

bool locate_dependency_environment_path(dependency_locator& self)
{
	string_handle const& dependency = *self.m_dependency;
	std::filesystem::path& tmp_path = self.m_tmp_path;
	std::array<wchar_t, 32 * 1024> buff;
	DWORD const got_env = GetEnvironmentVariableW(L"PATH", buff.data(), static_cast<DWORD>(buff.size()));
	assert(got_env != 0);
	assert(got_env < static_cast<DWORD>(buff.size()));
	auto const buff_end = buff.begin() + got_env;
	auto start = buff.begin();
	for(;;)
	{
		auto const it = std::find(start, buff_end, L';');
		tmp_path.assign(start, it).append(begin(dependency), end(dependency));
		if(std::filesystem::exists(tmp_path))
		{
			self.m_result = tmp_path;
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
