#include "allocator.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

#include "my_windows.h"


allocator::allocator() noexcept :
	m_small(),
	m_big()
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
	swap(m_small, other.m_small);
	swap(m_big, other.m_big);
}

void* allocator::allocate_bytes(int const size, int const align)
{
	assert(align <= alignof(std::max_align_t));
	if(size < 64 * 1024)
	{
		return m_small.allocate_bytes(size, align);
	}
	else
	{
		return m_big.allocate_bytes(size, align);
	}
}
