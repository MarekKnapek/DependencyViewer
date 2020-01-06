#pragma once


#include <string>
#include <vector>


typedef std::vector<std::wstring> manifests_t;


namespace default_manifests
{
	void init();
	void deinit();
	manifests_t const& get();
};
