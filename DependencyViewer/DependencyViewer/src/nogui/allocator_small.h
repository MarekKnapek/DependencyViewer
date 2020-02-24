#pragma once


#include <cstdint>


class allocator_small
{
private:
	struct header
	{
		std::uint32_t m_used;
		header* m_prev;
	};
public:
	allocator_small() noexcept;
	allocator_small(allocator_small const&) = delete;
	allocator_small(allocator_small&& other) noexcept;
	allocator_small& operator=(allocator_small const&) = delete;
	allocator_small& operator=(allocator_small&& other) noexcept;
	~allocator_small() noexcept;
	void swap(allocator_small& other) noexcept;
public:
	void* allocate_bytes(std::uint32_t const size, std::uint32_t const align);
private:
	header* find_block(std::uint32_t const size, std::uint32_t const align);
	header* allocate_block();
	void commit_memory(header* const block, std::uint32_t const alloc_size);
	void* allocate_from_block(header* const block, std::uint32_t const size, std::uint32_t const align);
private:
	header* m_state;
};

inline void swap(allocator_small& a, allocator_small& b) noexcept { a.swap(b); }
