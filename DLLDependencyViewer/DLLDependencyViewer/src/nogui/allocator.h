#pragma once


#include "allocator_big.h"
#include "allocator_small.h"


class allocator
{
public:
	allocator() noexcept;
	allocator(allocator const&) = delete;
	allocator(allocator&& other) noexcept;
	allocator& operator=(allocator const&) = delete;
	allocator& operator=(allocator&& other) noexcept;
	~allocator() noexcept;
	void swap(allocator& other) noexcept;
public:
	void* allocate_bytes(int const size, int const align);
	template<typename T> T* allocate_objects(int const size) { return static_cast<T*>(allocate_bytes(size * sizeof(T), alignof(T))); }
private:
	allocator_small m_small;
	allocator_big m_big;
};

inline void swap(allocator& a, allocator& b) noexcept { a.swap(b); }
