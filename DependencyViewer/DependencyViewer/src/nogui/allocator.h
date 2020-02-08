#pragma once


#define WANT_STANDARD_ALLOCATOR 0


#if WANT_STANDARD_ALLOCATOR == 1
#include "allocator_malloc.h"
#else
#include "allocator_big.h"
#include "allocator_small.h"
#endif


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
	#if WANT_STANDARD_ALLOCATOR == 1
	allocator_malloc m_mallocator;
	#else
	allocator_small m_small;
	allocator_big m_big;
	#endif
};

inline void swap(allocator& a, allocator& b) noexcept { a.swap(b); }
