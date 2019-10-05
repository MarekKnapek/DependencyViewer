#pragma once


void* allocator2__allocate_bytes(void** alloc_2, int const size, int const align);
void allocator2__deallocate_all(void** alloc_2);


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


class allocator
{
public:
	allocator() noexcept;
	allocator(allocator const&) = delete;
	allocator(allocator&& other) noexcept;
	allocator& operator=(allocator const&) = delete;
	allocator& operator=(allocator&& other) noexcept;
	~allocator() noexcept;
	void swap(allocator& other) noexcept;
public:
	void* allocate_bytes(int const& size, int const& align);
	template<typename T> T* allocate_objects(int const& size){ return static_cast<T*>(allocate_bytes(size * sizeof(T), alignof(T))); }
private:
	void* m_alc;
};

inline void swap(allocator& a, allocator& b) noexcept { a.swap(b); }
