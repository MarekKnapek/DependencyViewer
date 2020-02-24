#include "allocator_small.h"

#include "cassert_my.h"

#include <cstddef>
#include <utility>

#include "windows_my.h"


static constexpr std::uint32_t const s_chunk_size = 64 * 1024 * 1024;
static constexpr std::uint32_t const s_page_size = 4 * 1024;
static constexpr std::uint32_t const s_treshold = 64 * 1024;


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
	header* block = m_state;
	while(block)
	{
		header* const old_block = block;
		block = block->m_prev;
		BOOL const freed = VirtualFree(old_block, 0, MEM_RELEASE);
		assert(freed != 0);
	}
}

void allocator_small::swap(allocator_small& other) noexcept
{
	using std::swap;
	swap(m_state, other.m_state);
}

void* allocator_small::allocate_bytes(std::uint32_t const size, std::uint32_t const align)
{
	assert(size < s_treshold);
	assert(align <= alignof(std::max_align_t));
	header* block = find_block(size, align);
	if(!block)
	{
		[[unlikely]]
		block = allocate_block();
		assert(block);
		block->m_prev = m_state;
		m_state = block;
	}
	void* const ret = allocate_from_block(block, size, align);
	assert(ret);
	return ret;
}

allocator_small::header* allocator_small::find_block(std::uint32_t const size, std::uint32_t const align)
{
	std::uint32_t const needed = size + align - 1;
	header* block = m_state;
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

allocator_small::header* allocator_small::allocate_block()
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

void allocator_small::commit_memory(header* const block, std::uint32_t const alloc_size)
{
	assert(block);
	std::uint32_t const old_page = ((block->m_used + 0 - 1) / s_page_size) * s_page_size;
	std::uint32_t const new_page = ((block->m_used + alloc_size - 1) / s_page_size) * s_page_size;
	if(new_page != old_page)
	{
		[[maybe_unused]] void* const commited = VirtualAlloc(reinterpret_cast<char*>(block) + block->m_used, alloc_size, MEM_COMMIT, PAGE_READWRITE);
		assert(reinterpret_cast<std::uintptr_t>(commited) == ((reinterpret_cast<std::uintptr_t>(block) + block->m_used) / s_page_size) * s_page_size);
	}
}

void* allocator_small::allocate_from_block(header* const block, std::uint32_t const size, std::uint32_t const align)
{
	assert(block);
	std::uint32_t const needed = size + align - 1;
	std::uint32_t const available = s_chunk_size - block->m_used;
	assert(available >= needed);
	char* const begin = reinterpret_cast<char*>(block) + block->m_used;
	std::uint32_t i = 0;
	while((reinterpret_cast<std::uintptr_t>(begin + i) % align) != 0)
	{
		++i;
	};
	assert(i < align);
	char* const ret = begin + i;
	std::uint32_t const this_alloc_size = size + i;
	commit_memory(block, this_alloc_size);
	block->m_used += this_alloc_size;
	assert(block->m_used <= s_chunk_size);
	return ret;
}
