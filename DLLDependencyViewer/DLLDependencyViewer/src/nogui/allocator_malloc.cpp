#include "allocator_malloc.h"

#include <vector>
#include <cstdlib>
#include <cassert>


allocator_malloc::allocator_malloc() noexcept :
	m_state(nullptr)
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
	if(!m_state)
	{
		return;
	}
	std::vector<void*>* vec = static_cast<std::vector<void*>*>(m_state);
	auto const end = vec->rend();
	for(auto it = vec->rbegin(); it != end; ++it)
	{
		(std::free)(*it);
	}
	delete vec;
	m_state = nullptr;
}

void allocator_malloc::swap(allocator_malloc& other) noexcept
{
	using std::swap;
	swap(m_state, other.m_state);
}

void* allocator_malloc::allocate_bytes(int const size, int const align)
{
	assert(align <= alignof(std::max_align_t));
	(void)align;
	if(!m_state)
	{
		m_state = new std::vector<void*>();
	}
	std::vector<void*>& vec = *static_cast<std::vector<void*>*>(m_state);
	void* const mem = (std::malloc)(size);
	vec.push_back(mem);
	return mem;
}
