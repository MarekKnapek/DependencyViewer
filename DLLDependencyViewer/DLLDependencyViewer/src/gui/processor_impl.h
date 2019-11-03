#pragma once


#include "processor.h"

#include <cstddef>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>


struct enpt_t
{
	std::uint16_t const* m_enpt;
	std::uint16_t m_count;
};

struct file_info_big
{
	file_info* m_fi;
	enpt_t m_enpt;
};

struct processor_impl
{
	main_type* m_mo = nullptr;
	std::queue<file_info*> m_queue;
	std::unordered_map<string_handle, file_info_big, string_handle_case_insensitive_hash, string_handle_case_insensitive_equal> m_map;
	manifest_parser* m_manifest_parser = nullptr;
	enpt_t* m_curr_enpt;
	allocator m_entp_alloc;
};


main_type process_impl(std::vector<std::wstring> const& file_paths);
void process_r(processor_impl& prcsr);
void process_e(processor_impl& prcsr, file_info& fi, file_info& sub_fi, string_handle const& dll_name);
manifest_data process_manifest(processor_impl& prcsr, file_info const& fi);
std::pair<std::byte const*, int> find_manifest(file_info const& fi);
void pair_imports_with_exports(processor_impl& prcsr, file_info& fi, file_info& sub_fi);
void pair_exports_with_imports(processor_impl& prcsr, file_info& fi, file_info& sub_fi);
