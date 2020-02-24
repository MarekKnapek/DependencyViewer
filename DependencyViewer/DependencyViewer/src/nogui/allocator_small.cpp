#include "allocator_small.h"

#include "cassert_my.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "windows_my.h"


struct header
{
	std::uint32_t m_used;
	header* m_prev;
};


static constexpr std::uint32_t const s_chunk_size = 64 * 1024 * 1024;
static constexpr std::uint32_t const s_page_size = 4 * 1024;


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
	std::uint32_t const needed = size + align - 1;
	header* self = static_cast<header*>(m_state);
	header* block = self;
	while(block)
	{
		std::uint32_t const available = s_chunk_size - block->m_used;
		if(available >= needed)
		{
			return block;
		}
		block = block->m_prev;
	};
	return nullptr;
}

void* allocator_small::allocate_block()
{
	void* const new_mem = VirtualAlloc(nullptr, s_chunk_size, MEM_RESERVE, PAGE_READWRITE);
	assert(new_mem);
	[[maybe_unused]] void* const new_mem2 = VirtualAlloc(new_mem, static_cast<int>(sizeof(header)), MEM_COMMIT, PAGE_READWRITE);
	assert(new_mem2 == new_mem);
	header* const block = static_cast<header*>(new_mem);
	block->m_used = static_cast<int>(sizeof(header));
	block->m_prev = nullptr;
	return block;
}

void* allocator_small::allocate_from_block(void* const block, int const size, int const align)
{
	assert(block);
	std::uint32_t const sz = size;
	std::uint32_t const algn = align;
	header* const self = static_cast<header*>(block);
	std::uint32_t const needed = sz + algn - 1;
	std::uint32_t const available = s_chunk_size - self->m_used;
	assert(available >= needed);
	char* const begin = reinterpret_cast<char*>(self) + self->m_used;
	std::uint32_t i = 0;
	while((reinterpret_cast<std::uintptr_t>(begin + i) % algn) != 0)
	{
		++i;
	};
	char* const ret = begin + i;
	assert(i < algn);
	std::uint32_t const this_alloc_size = sz + i;
	commit_memory(self, this_alloc_size);
	self->m_used += this_alloc_size;
	assert(self->m_used <= s_chunk_size);
	return ret;
}

void allocator_small::commit_memory(void* const block, int const alloc_size)
{
	assert(block);
	header* const self = static_cast<header*>(block);
	std::uint32_t const allc_sz = alloc_size;
	std::uint32_t const old_page = ((self->m_used - 1) / s_page_size) * s_page_size;
	std::uint32_t const new_page = ((self->m_used + allc_sz - 1) / s_page_size) * s_page_size;
	if(old_page != new_page)
	{
		[[maybe_unused]] void* const commited = VirtualAlloc(static_cast<char*>(block) + self->m_used, allc_sz, MEM_COMMIT, PAGE_READWRITE);
		assert(reinterpret_cast<std::uintptr_t>(commited) == ((reinterpret_cast<std::uintptr_t>(block) + self->m_used) / s_page_size) * s_page_size);
	}
}
