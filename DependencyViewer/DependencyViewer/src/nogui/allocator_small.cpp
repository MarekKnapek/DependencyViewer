#include "allocator_small.h"

#include "cassert_my.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "my_windows.h"


struct header
{
	int m_remaining;
	header* m_prev;
};


static constexpr int const s_chunk_size = 2 * 1024 * 1024;
static constexpr int const s_chunk_usable_size = s_chunk_size - sizeof(header);


allocator_small::allocator_small() noexcept :
	m_state(nullptr)
{
}

allocator_small::allocator_small(allocator_small&& other) noexcept :
	allocator_small()
{
	swap(other);
}

allocator_small& allocator_small::operator=(allocator_small&& other) noexcept
{
	swap(other);
	return *this;
}

allocator_small::~allocator_small() noexcept
{
	header* self = static_cast<header*>(m_state);
	while(self)
	{
		header* const old_self = self;
		self = self->m_prev;
		BOOL const freed = VirtualFree(old_self, 0, MEM_RELEASE);
		assert(freed != 0);
	}
}

void allocator_small::swap(allocator_small& other) noexcept
{
	using std::swap;
	swap(m_state, other.m_state);
}

void* allocator_small::allocate_bytes(int const size, int const align)
{
	assert(size < 64 * 1024);
	assert(align <= alignof(std::max_align_t));
	header* block = static_cast<header*>(find_block(size, align));
	if(!block)
	{
		block = static_cast<header*>(allocate_block());
		assert(block);
		header* const self = static_cast<header*>(m_state);
		block->m_prev = self;
		m_state = block;
	}
	return allocate_from_block(block, size, align);
}

void* allocator_small::find_block(int const size, int const align)
{
	int const needed = size + align - 1;
	header* self = static_cast<header*>(m_state);
	header* block = self;
	while(block)
	{
		if(block->m_remaining >= needed)
		{
			return block;
		}
		block = block->m_prev;
	};
	return nullptr;
}

void* allocator_small::allocate_block()
{
	void* const new_mem = VirtualAlloc(NULL, s_chunk_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(new_mem);
	header* const block = static_cast<header*>(new_mem);
	block->m_remaining = s_chunk_usable_size;
	block->m_prev = nullptr;
	return block;
}

void* allocator_small::allocate_from_block(void* const block, int const size, int const align)
{
	assert(block);
	header* const self = static_cast<header*>(block);
	int const needed = size + align - 1;
	assert(self->m_remaining >= needed);
	int const already_consumed = s_chunk_usable_size - self->m_remaining;
	char* const begin = reinterpret_cast<char*>(self + 1) + already_consumed;
	int i = 0;
	while((reinterpret_cast<std::uintptr_t>(begin + i) % align) != 0)
	{
		++i;
	};
	char* const ret = begin + i;
	assert(i < align);
	int const this_alloc_size = size + i;
	self->m_remaining -= this_alloc_size;
	assert(self->m_remaining >= 0);
	return ret;
}
