#include "allocator_big.h"

#include "cassert_my.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>

#include "my_windows.h"


static constexpr int const s_allocator_big_state_size = 64 * 1024;


struct allocator_big_inner_t;
struct allocator_big_outer_t;


struct allocator_big_inner_t
{
	int m_free_allocs;
	allocator_big_outer_t* m_prev;
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
	allocator_big_outer_t* self = static_cast<allocator_big_outer_t*>(m_state);
	while(self)
	{
		allocator_big_outer_t* const old_self = self;
		self = self->m_inner.m_prev;
		int const used_allocs = static_cast<int>(std::size(old_self->m_allocs)) - old_self->m_inner.m_free_allocs;
		for(int i = 0; i != used_allocs; ++i)
		{
			BOOL const freed = VirtualFree(old_self->m_allocs[i], 0, MEM_RELEASE);
			assert(freed != 0);
		}
		BOOL const freed = VirtualFree(old_self, 0, MEM_RELEASE);
		assert(freed != 0);
	}
}

void allocator_big::swap(allocator_big& other) noexcept
{
	using std::swap;
	swap(m_state, other.m_state);
}

void* allocator_big::allocate_bytes(int const size, [[maybe_unused]] int const align)
{
	assert(size >= 64 * 1024);
	assert(align <= alignof(std::max_align_t));
	allocator_big_outer_t* self = static_cast<allocator_big_outer_t*>(m_state);
	if(!self || self->m_inner.m_free_allocs == 0)
	{
		void* const new_mem_1 = VirtualAlloc(NULL, s_allocator_big_state_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		assert(new_mem_1);
		allocator_big_outer_t* const state_1 = static_cast<allocator_big_outer_t*>(new_mem_1);
		state_1->m_inner.m_free_allocs = static_cast<int>(std::size(state_1->m_allocs));
		state_1->m_inner.m_prev = self;
		m_state = state_1;
		self = state_1;
	}
	assert(self);
	assert(self->m_inner.m_free_allocs > 0);
	void* const new_mem_2 = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(new_mem_2);
	self->m_allocs[std::size(self->m_allocs) - self->m_inner.m_free_allocs] = new_mem_2;
	--self->m_inner.m_free_allocs;
	return new_mem_2;
}
