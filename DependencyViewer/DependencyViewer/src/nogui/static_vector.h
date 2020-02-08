#pragma once


class allocator;


template<typename T>
class static_vector
{
public:
	static_vector() noexcept;
	static_vector(static_vector const&) = delete;
	static_vector(static_vector&& other) noexcept;
	static_vector<T>& operator=(static_vector<T> const&) = delete;
	static_vector<T>& operator=(static_vector<T>&& other) noexcept;
	void swap(static_vector<T>& other) noexcept;
	~static_vector();
public:
	void resize(allocator& alc, int const size);
public:
	bool empty() const;
	T& operator[](int const idx);
	T const& operator[](int const idx) const;
	T* data();
	T const* data() const;
	int size() const;
	T* begin();
	T const* begin() const;
	T const* cbegin() const;
	T& front();
	T const& front() const;
	T* end();
	T const* end() const;
	T const* cend() const;
private:
	T* m_data;
	int m_size;
};

template<typename T> inline void swap(static_vector<T>& a, static_vector<T>& b) noexcept { a.swap(b); }
template<typename T> inline int size(static_vector<T> const& vec) { return vec.size(); }
template<typename T> inline T* begin(static_vector<T>& vec) { return vec.begin(); }
template<typename T> inline T const* begin(static_vector<T> const& vec) { return vec.begin(); }
template<typename T> inline T const* cbegin(static_vector<T> const& vec) { return vec.cbegin(); }
template<typename T> inline T* end(static_vector<T>& vec) { return vec.end(); }
template<typename T> inline T const* end(static_vector<T> const& vec) { return vec.end(); }
template<typename T> inline T const* cend(static_vector<T> const& vec) { return vec.cend(); }


#include "static_vector.inl"
