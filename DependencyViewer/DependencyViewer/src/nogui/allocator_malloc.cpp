#include "allocator_malloc.h"

#include "cassert_my.h"

#include <cstdlib>


allocator_malloc::allocator_malloc() noexcept :
	m_state()
{
}

allocator_malloc::allocator_malloc(allocator_malloc&& other) noexcept :
	allocator_malloc()
{
	swap(other);
}

allocator_malloc& allocator_malloc::operator=(allocator_malloc&& other) noexcept
{
	swap(other);
	return *this;
}

allocator_malloc::~allocator_malloc() noexcept
{
	auto const end = m_state.rend();
	for(auto it = m_state.rbegin(); it != end; ++it)
	{
		(std::free)(*it);
	}
	m_state.clear();
}

void allocator_malloc::swap(allocator_malloc& other) noexcept
{
	using std::swap;
	swap(m_state, other.m_state);
}

void* allocator_malloc::allocate_bytes(int const size, [[maybe_unused]] int const align)
{
	assert(align <= alignof(std::max_align_t));
	void* const mem = (std::malloc)(size);
	m_state.push_back(mem);
	return mem;
}
