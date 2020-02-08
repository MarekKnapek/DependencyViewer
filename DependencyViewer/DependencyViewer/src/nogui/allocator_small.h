#pragma once


class allocator_small
{
public:
	allocator_small() noexcept;
	allocator_small(allocator_small const&) = delete;
	allocator_small(allocator_small&& other) noexcept;
	allocator_small& operator=(allocator_small const&) = delete;
	allocator_small& operator=(allocator_small&& other) noexcept;
	~allocator_small() noexcept;
	void swap(allocator_small& other) noexcept;
public:
	void* allocate_bytes(int const size, int const align);
private:
	void* find_block(int const size, int const align);
	void* allocate_block();
	void* allocate_from_block(void* const block, int const size, int const align);
private:
	void* m_state;
};

inline void swap(allocator_small& a, allocator_small& b) noexcept { a.swap(b); }
