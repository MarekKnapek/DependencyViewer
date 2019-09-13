#pragma once


#include "processor.h"

#include "../nogui/file_name.h"

#include <string>
#include <queue>
#include <unordered_map>


struct processor_impl
{
	main_type* m_mo = nullptr;
	std::wstring const* m_main_file_path;
	std::queue<file_info*> m_queue;
	std::unordered_map<string const*, file_info*, string_case_insensitive_hash, string_case_insensitive_equal> m_map;
	file_name* m_file_name;
	manifest_parser* m_manifest_parser;
};


main_type process_impl(std::wstring const& main_file_path);
void process_r(processor_impl& prcsr);
void process_e(processor_impl& prcsr, file_info& fi, string const* const& dll_name);
manifest_data process_manifest(processor_impl& prcsr, file_info const& fi);
std::pair<char const*, int> find_manifest(file_info const& fi);
