#include "memory_mapped_file.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include <windows.h>


static constexpr int const s_max_file_size = 1 * 1024 * 1024 * 1024;
static constexpr wchar_t const s_mmf_open[] = L"Could not open file.";
static constexpr wchar_t const s_mmf_empty[] = L"File is empty.";
static constexpr wchar_t const s_mmf_big[] = L"File too big, maximum file size is 1024 MB.";
static constexpr wchar_t const s_mmf_mapping[] = L"Failed to create file mapping.";
static constexpr wchar_t const s_mmf_view[] = L"Failed to create file view.";


void mapped_view_deleter::operator()(void const* const ptr) const
{
	BOOL const unmapped = UnmapViewOfFile(ptr);
	assert(unmapped != 0);
}


memory_mapped_file::memory_mapped_file() noexcept :
	m_file(),
	m_mapping(),
	m_view(),
	m_end()
{
}

memory_mapped_file::memory_mapped_file(wchar_t const* const file_name) :
	memory_mapped_file()
{
	HANDLE const file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file == INVALID_HANDLE_VALUE)
	{
		throw s_mmf_open;
	}
	smart_handle s_file(file);
	LARGE_INTEGER size;
	BOOL const got_size = GetFileSizeEx(file, &size);
	assert(got_size != 0);
	if(size.HighPart != 0)
	{
		throw s_mmf_big;
	}
	if(size.LowPart == 0)
	{
		throw s_mmf_empty;
	}
	if(size.LowPart > s_max_file_size)
	{
		throw s_mmf_big;
	}
	HANDLE const mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if(mapping == nullptr)
	{
		throw s_mmf_mapping;
	}
	smart_handle s_mapping(mapping);
	void const* const ptr = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
	if(!ptr)
	{
		throw s_mmf_view;
	}
	smart_mapped_view s_view(ptr);

	m_file = std::move(s_file);
	m_mapping = std::move(s_mapping);
	m_view = std::move(s_view);
	m_end = static_cast<char const*>(m_view.get()) + static_cast<int>(size.LowPart);
}

memory_mapped_file::memory_mapped_file(memory_mapped_file&& other) noexcept :
	memory_mapped_file()
{
	swap(other);
}

memory_mapped_file& memory_mapped_file::operator=(memory_mapped_file&& other) noexcept
{
	swap(other);
	return *this;
}

memory_mapped_file::~memory_mapped_file() noexcept
{
}

void memory_mapped_file::swap(memory_mapped_file& other) noexcept
{
	using std::swap;
	swap(m_file, other.m_file);
	swap(m_mapping, other.m_mapping);
	swap(m_view, other.m_view);
	swap(m_end, other.m_end);
}

void const* memory_mapped_file::begin() const
{
	return m_view.get();
}

void const* memory_mapped_file::end() const
{
	return m_end;
}

int memory_mapped_file::size() const
{
	return static_cast<int>(static_cast<char const*>(end()) - static_cast<char const*>(begin()));
}
