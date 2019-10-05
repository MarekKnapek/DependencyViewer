#include "allocator.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

#include "my_windows.h"


#define WANT_CUSTOM_ALLOCATOR 1


struct header
{
	int m_remaining;
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
		(*self)->m_remaining = s_chunk_usable_size;
		(*self)->m_prev = nullptr;
		(*self)->m_user = nullptr;
	}
	int const needed = size + align - 1;
	assert(needed <= s_chunk_usable_size);
	if(needed > (*self)->m_remaining)
	{
		void* const new_mem = VirtualAlloc(NULL, s_chunk_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		assert(new_mem);
		header* const new_self = static_cast<header*>(new_mem);
		new_self->m_remaining = s_chunk_usable_size;
		new_self->m_prev = *self;
		new_self->m_user = (*self)->m_user;
		*self = new_self;
	}
	int const consumed =  s_chunk_usable_size - (*self)->m_remaining;
	char* begin = reinterpret_cast<char*>(*self + 1) + consumed;
	int i = 0;
	while((reinterpret_cast<std::uintptr_t>(begin) % align) != 0)
	{
		++i;
		++begin;
	};
	assert(i < align);
	(*self)->m_remaining -= size + i;
	assert((*self)->m_remaining >= 0);
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


static constexpr int const s_allocator_big_state_size = 64 * 1024;


struct allocator_big_inner_t
{
	int m_used;
	void* m_next;
};

struct allocator_big_outer_t
{
	allocator_big_inner_t m_inner;
	void* m_allocs[(s_allocator_big_state_size - sizeof(allocator_big_inner_t)) / sizeof(void*)];
};


allocator_big::allocator_big() noexcept :
 m_state(nullptr)
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
	allocator_big_outer_t* state = reinterpret_cast<allocator_big_outer_t*>(m_state);
	while(state)
	{
		allocator_big_outer_t* const state_old = state;
		state = reinterpret_cast<allocator_big_outer_t*>(state->m_inner.m_next);
		for(int i = 0; i != state_old->m_inner.m_used; ++i)
		{
			BOOL const freed = VirtualFree(state_old->m_allocs[i], 0, MEM_RELEASE);
			assert(freed != 0);
		}
		BOOL const freed = VirtualFree(state_old, 0, MEM_RELEASE);
		assert(freed != 0);
	}
}

void allocator_big::swap(allocator_big& other) noexcept
{
	using std::swap;
	swap(m_state, other.m_state);
}

void* allocator_big::allocate_bytes(int const size, int const align)
{
	assert(size >= s_big_allocation_threshold);
	assert(align <= alignof(std::max_align_t));
	(void)align;
	if(!m_state)
	{
		void* const new_mem_1 = VirtualAlloc(NULL, s_allocator_big_state_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		assert(new_mem_1);
		allocator_big_outer_t* const state_1 = reinterpret_cast<allocator_big_outer_t*>(new_mem_1);
		assert(state_1->m_inner.m_used == 0);
		assert(state_1->m_inner.m_next == nullptr);
		assert(std::all_of(std::cbegin(state_1->m_allocs), std::cend(state_1->m_allocs), [](void* const& e){ return e == nullptr; }));
		m_state = state_1;
	}
	{
		allocator_big_outer_t* const state_2 = reinterpret_cast<allocator_big_outer_t*>(m_state);
		if(state_2->m_inner.m_used == static_cast<int>(std::size(state_2->m_allocs)))
		{
			void* const new_mem_2 = VirtualAlloc(NULL, s_allocator_big_state_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			assert(new_mem_2);
			allocator_big_outer_t* const state_3 = reinterpret_cast<allocator_big_outer_t*>(new_mem_2);
			assert(state_3->m_inner.m_used == 0);
			assert(state_3->m_inner.m_next == nullptr);
			assert(std::all_of(std::cbegin(state_3->m_allocs), std::cend(state_3->m_allocs), [](void* const& e){ return e == nullptr; }));
			state_3->m_inner.m_next = state_2;
			m_state = state_3;
		}
	}
	allocator_big_outer_t* const state_4 = reinterpret_cast<allocator_big_outer_t*>(m_state);
	void* const new_mem_3 = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(new_mem_3);
	state_4->m_allocs[state_4->m_inner.m_used] = new_mem_3;
	++state_4->m_inner.m_used;
	return new_mem_3;
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
