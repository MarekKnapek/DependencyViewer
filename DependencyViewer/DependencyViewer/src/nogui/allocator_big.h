#pragma once


class allocator_big
{
public:
	allocator_big() noexcept;
	allocator_big(allocator_big const&) = delete;
	allocator_big(allocator_big&& other) noexcept;
	allocator_big& operator=(allocator_big const&) = delete;
	allocator_big& operator=(allocator_big&& other) noexcept;
	~allocator_big() noexcept;
	void swap(allocator_big& other) noexcept;
public:
	void* allocate_bytes(int const size, int const align);
private:
	void* m_state;
};

inline void swap(allocator_big& a, allocator_big& b) noexcept { a.swap(b); }
