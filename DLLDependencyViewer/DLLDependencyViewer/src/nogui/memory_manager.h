#pragma once


#include "allocator.h"
#include "unique_strings.h"


class memory_manager
{
public:
	memory_manager() noexcept;
	memory_manager(memory_manager const&) = delete;
	memory_manager(memory_manager&& other) noexcept;
	memory_manager& operator=(memory_manager const&) = delete;
	memory_manager& operator=(memory_manager&& other) noexcept;
	~memory_manager() noexcept;
	void swap(memory_manager& other) noexcept;
public:
	allocator m_alc;
	unique_strings m_strs;
	wunique_strings m_wstrs;
};

inline void swap(memory_manager& a, memory_manager& b) noexcept { a.swap(b); }
