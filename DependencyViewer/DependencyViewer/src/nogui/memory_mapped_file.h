#pragma once


#include "smart_handle.h"

#include <cstddef>
#include <memory>


struct mapped_view_deleter
{
public:
	void operator()(void const* const ptr) const;
};
typedef std::unique_ptr<void const, mapped_view_deleter> smart_mapped_view;


class memory_mapped_file
{
public:
	memory_mapped_file() noexcept;
	memory_mapped_file(wchar_t const* const file_name);
	memory_mapped_file(memory_mapped_file const&) = delete;
	memory_mapped_file(memory_mapped_file&& other) noexcept;
	memory_mapped_file& operator=(memory_mapped_file const&) = delete;
	memory_mapped_file& operator=(memory_mapped_file&& other) noexcept;
	~memory_mapped_file() noexcept;
	void swap(memory_mapped_file& other) noexcept;
public:
	std::byte const* begin() const;
	std::byte const* end() const;
	int size() const;
private:
	smart_handle m_file;
	smart_handle m_mapping;
	smart_mapped_view m_view;
	int m_size;
};

inline void swap(memory_mapped_file& a, memory_mapped_file& b) noexcept { a.swap(b); }
