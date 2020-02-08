#pragma once


#include "processor.h"

#include "../nogui/allocator.h"
#include "../nogui/dependency_locator.h"
#include "../nogui/memory_manager.h"
#include "../nogui/my_string_handle.h"

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>


struct enptr_type
{
	std::uint16_t const* m_table;
	std::uint16_t m_count;
};

struct fat_type
{
	file_info* m_instance;
	enptr_type m_enpt;
};

struct fat_type_hash
{
	std::size_t operator()(fat_type const* const& obj) const
	{
		return std::hash<wstring_handle>{}(obj->m_instance->m_file_path);
	}
};

struct fat_type_eq
{
	bool operator()(fat_type const* const& a, fat_type const* const& b) const
	{
		return a->m_instance->m_file_path == b->m_instance->m_file_path;
	}
};

struct tmp_type
{
	main_type* m_mo;
	memory_manager* m_mm;
	allocator m_tmp_alc;
	std::deque<file_info*> m_queue;
	std::unordered_set<fat_type*, fat_type_hash, fat_type_eq> m_map;
	dependency_locator m_dl;
};


bool process_impl(std::vector<std::wstring> const& file_paths, main_type& mo);

void make_doubly_linked_list(file_info& fi);
modules_list_t make_modules_list(tmp_type const& to);

bool step_1(tmp_type& to);
bool step_2(file_info& fi, tmp_type& to);
bool step_3(file_info const& fi, std::uint16_t const i, tmp_type& to);
