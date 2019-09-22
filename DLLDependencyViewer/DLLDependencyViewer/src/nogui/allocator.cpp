#include "allocator.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

#include <windows.h>


#define WANT_CUSTOM_ALLOCATOR 1


struct header
{
	int m_used;
	header* m_prev;
	void* m_user;
};


static constexpr int const s_chunk_size = 2 * 1024 * 1024;
static constexpr int const s_chunk_usable_size = s_chunk_size - sizeof(header);
#if WANT_CUSTOM_ALLOCATOR == 1
static constexpr int const s_big_allocation_threshold = 64 * 1024;
#else
static constexpr int const s_big_allocation_threshold = 0;
#endif


void* allocate_bytes_2(void** alloc_2, int const& size, int const& align)
{
	assert(size < s_big_allocation_threshold || size == sizeof(allocator_big));
	assert(align <= alignof(std::max_align_t));
	header** self = reinterpret_cast<header**>(alloc_2);
	if(!*self)
	{
		void* const new_mem = VirtualAlloc(NULL, s_chunk_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		assert(new_mem);
		*self = static_cast<header*>(new_mem);
	}
	int const remaining = s_chunk_usable_size - (*self)->m_used;
	int const needed = size + align - 1;
	assert(needed <= s_chunk_usable_size);
	if(needed > remaining)
	{
		void* const new_mem = VirtualAlloc(NULL, s_chunk_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		assert(new_mem);
		header* const new_self = static_cast<header*>(new_mem);
		new_self->m_prev = *self;
		new_self->m_user = (*self)->m_user;
		*self = new_self;
	}
	char* begin = reinterpret_cast<char*>(*self + 1) + (*self)->m_used;
	int i = 0;
	while((reinterpret_cast<std::uintptr_t>(begin) % align) != 0)
	{
		++i;
		++begin;
	};
	assert(i < align);
	(*self)->m_used += size + i;
	assert((*self)->m_used <= s_chunk_usable_size);
	return begin;
}

void deallocate_all_2(void** alloc_2)
{
	header** const self = reinterpret_cast<header**>(alloc_2);
	while(*self)
	{
		header* const prev_self = (*self)->m_prev;
		BOOL const freed = VirtualFree(*self, 0, MEM_RELEASE);
		assert(freed != 0);
		*self = prev_self;
	}
}


allocator_big::allocator_big() noexcept :
 m_allocs()
{
}

allocator_big::allocator_big(allocator_big&& other) noexcept :
	allocator_big()
{
	swap(other);
}

allocator_big& allocator_big::operator=(allocator_big&& other) noexcept
{
	swap(other);
	return *this;
}

allocator_big::~allocator_big() noexcept
{
	for(auto const& alloc : m_allocs)
	{
		std::free(alloc);
	}
}

void allocator_big::swap(allocator_big& other) noexcept
{
	using std::swap;
	swap(m_allocs, other.m_allocs);
}

void* allocator_big::allocate_bytes(int const& size, int const& align)
{
	assert(size >= s_big_allocation_threshold);
	assert(align <= alignof(std::max_align_t));
	(void)align;
	void* const alloc = std::malloc(size);
	m_allocs.push_back(alloc);
	return alloc;
}


allocator::allocator() noexcept :
	m_alc()
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
	if(m_alc && static_cast<header*>(m_alc)->m_user)
	{
		static_cast<allocator_big*>(static_cast<header*>(m_alc)->m_user)->~allocator_big();
	}
	deallocate_all_2(&m_alc);
}

void allocator::swap(allocator& other) noexcept
{
	using std::swap;
	swap(m_alc, other.m_alc);
}

void* allocator::allocate_bytes(int const& size, int const& align)
{
	assert(align <= alignof(std::max_align_t));
	if(size >= s_big_allocation_threshold)
	{
		if(m_alc == nullptr || static_cast<header*>(m_alc)->m_user == nullptr)
		{
			void* const alc_big = allocate_bytes_2(&m_alc, sizeof(allocator_big), alignof(allocator_big));
			::new(alc_big)allocator_big;
			static_cast<header*>(m_alc)->m_user = alc_big;
		}
		return static_cast<allocator_big*>(static_cast<header*>(m_alc)->m_user)->allocate_bytes(size, align);
	}
	else
	{
		return allocate_bytes_2(&m_alc, size, align);
	}
}
