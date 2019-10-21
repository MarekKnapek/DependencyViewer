#pragma once


#include <vector>


class allocator_malloc
{
public:
	allocator_malloc() noexcept;
	allocator_malloc(allocator_malloc const&) = delete;
	allocator_malloc(allocator_malloc&& other) noexcept;
	allocator_malloc& operator=(allocator_malloc const&) = delete;
	allocator_malloc& operator=(allocator_malloc&& other) noexcept;
	~allocator_malloc() noexcept;
	void swap(allocator_malloc& other) noexcept;
public:
	void* allocate_bytes(int const size, int const align);
private:
	std::vector<void*> m_state;
};

inline void swap(allocator_malloc& a, allocator_malloc& b) noexcept { a.swap(b); }
