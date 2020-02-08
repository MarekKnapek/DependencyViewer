#pragma once


#include <string>
#include <vector>


namespace known_dlls
{
	void init();
	void deinit();
	std::wstring const& get_path();
	std::vector<std::wstring> const& get_names();
	std::vector<std::string> const& get_names_sorted_lowercase_ascii();
}
