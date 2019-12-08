#pragma once


#include <string>
#include <vector>


bool init_known_dlls();
void deinit_known_dlls();

std::wstring const& get_knonw_dlls_path();
std::vector<std::wstring> const& get_known_dll_names();
std::vector<std::string> const& get_known_dll_names_sorted_lowercase_ascii();
