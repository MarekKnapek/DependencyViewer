#include "allocator.h"

#include "cassert_my.h"

#include <algorithm>
#include <cstdint>

#include "my_windows.h"


allocator::allocator() noexcept :
	#if WANT_STANDARD_ALLOCATOR == 1
	m_mallocator()
	#else
	m_small(),
	m_big()
	#endif
{
}

allocator::allocator(allocator&& other) noexcept :
	allocator()
{
	swap(other);
}

allocator& allocator::operator=(allocator&& other) noexcept
{
	swap(other);
	return *this;
}

allocator::~allocator() noexcept
{
}

void allocator::swap(allocator& other) noexcept
{
	using std::swap;
	#if WANT_STANDARD_ALLOCATOR == 1
	swap(m_mallocator, other.m_mallocator);
	#else
	swap(m_small, other.m_small);
	swap(m_big, other.m_big);
	#endif
}

void* allocator::allocate_bytes(int const size, int const align)
{
	#if WANT_STANDARD_ALLOCATOR == 1
	return m_mallocator.allocate_bytes(size, align);
	#else
	assert(align <= alignof(std::max_align_t));
	if(size < 64 * 1024)
	{
		return m_small.allocate_bytes(size, align);
	}
	else
	{
		return m_big.allocate_bytes(size, align);
	}
	#endif
}
