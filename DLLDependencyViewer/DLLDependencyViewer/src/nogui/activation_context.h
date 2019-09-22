#pragma once


#include <string>
#include <vector>


typedef std::vector<std::wstring> manifests_t;


class activation_context
{
public:
	static const manifests_t& get_system_default_manifests();
	static void free_system_default_manifests();
private:
	static void const* get_system_default_activation_context_data();
};
