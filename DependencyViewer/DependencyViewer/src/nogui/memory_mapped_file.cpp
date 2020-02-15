#include "memory_mapped_file.h"

#include "assert_my.h"
#include "cassert_my.h"

#include <algorithm>
#include <utility>

#include "my_windows.h"


#define s_very_big_int (2'147'483'647)
static constexpr int const s_max_file_size = s_very_big_int;


void mapped_view_deleter::operator()(void const* const ptr) const
{
	BOOL const unmapped = UnmapViewOfFile(ptr);
	assert(unmapped != 0);
}


memory_mapped_file::memory_mapped_file() noexcept :
	m_file(),
	m_mapping(),
	m_view(),
	m_size()
{
}

memory_mapped_file::memory_mapped_file(wchar_t const* const file_name) :
	memory_mapped_file()
{
	HANDLE const file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	WARN_M_RV(file != INVALID_HANDLE_VALUE, L"Failed to CreateFileW.");
	smart_handle s_file(file);
	LARGE_INTEGER size;
	BOOL const got_size = GetFileSizeEx(file, &size);
	assert(got_size != 0);
	WARN_M_RV(size.HighPart == 0, L"File is too big.");
	WARN_M_RV(size.LowPart != 0, L"File is empty.");
	WARN_M_RV(size.LowPart <= s_max_file_size, L"File is too big.");
	HANDLE const mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
	WARN_M_RV(mapping != nullptr, L"Failed to CreateFileMappingW.");
	smart_handle s_mapping(mapping);
	void const* const ptr = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
	WARN_M_RV(mapping != nullptr, L"Failed to MapViewOfFile.");
	smart_mapped_view s_view(ptr);

	m_file = std::move(s_file);
	m_mapping = std::move(s_mapping);
	m_view = std::move(s_view);
	m_size = static_cast<int>(size.LowPart);
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
	swap(m_size, other.m_size);
}

std::byte const* memory_mapped_file::begin() const
{
	return static_cast<std::byte const*>(m_view.get());
}

std::byte const* memory_mapped_file::end() const
{
	return begin() + size();
}

int memory_mapped_file::size() const
{
	return m_size;
}
