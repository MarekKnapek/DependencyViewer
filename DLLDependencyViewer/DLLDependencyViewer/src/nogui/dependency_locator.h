#pragma once


#include "my_string_handle.h"

#include <filesystem>
#include <string>


struct dependency_locator
{
	wstring_handle m_main_path;
	string_handle const* m_dependency;
	std::wstring m_result;
	std::string m_tmpn;
	std::filesystem::path m_tmp_path;
};


bool locate_dependency(dependency_locator& self);

bool locate_dependency_sxs(dependency_locator& self);
bool locate_dependency_known_dlls(dependency_locator& self);
bool locate_dependency_application_dir(dependency_locator& self);
bool locate_dependency_system32(dependency_locator& self);
bool locate_dependency_system16(dependency_locator& self);
bool locate_dependency_windows(dependency_locator& self);
bool locate_dependency_current_dir(dependency_locator& self);
bool locate_dependency_environment_path(dependency_locator& self);
