#pragma once


#include "my_string_handle.h"


struct dependency_locator
{
	wstring_handle m_main_path;
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
