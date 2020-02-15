#include "static_vector.h"

#include "allocator.h"
#include "cassert_my.h"

#include <type_traits>


template<typename T>
static_vector<T>::static_vector() noexcept :
	m_data(nullptr),
	m_size(0)
{
}

template<typename T>
static_vector<T>::static_vector(static_vector&& other) noexcept :
	static_vector()
{
	swap(other);
}

template<typename T>
static_vector<T>& static_vector<T>::operator=(static_vector<T>&& other) noexcept
{
	swap(other);
	return *this;
}

template<typename T>
void static_vector<T>::swap(static_vector<T>& other) noexcept
{
	using std::swap;
	swap(m_data, other.m_data);
	swap(m_size, other.m_size);
}

template<typename T>
static_vector<T>::~static_vector()
{
	if constexpr(!std::is_trivial_v<T>)
	{
		for(int i = 0; i != m_size; ++i)
		{
			m_data[m_size - i - 1].~T();
		}
	}
}

template<typename T>
void static_vector<T>::resize(allocator& alc, int const size)
{
	assert(m_data == nullptr);
	assert(m_size == 0);
	m_data = alc.allocate_objects<T>(size);
	if constexpr(!std::is_trivial_v<T>)
	{
		for(int i = 0; i != size; ++i)
		{
			new(m_data + i) T();
		}
	}
	m_size = size;
}

template<typename T>
bool static_vector<T>::empty() const
{
	return m_size == 0;
}

template<typename T>
T& static_vector<T>::operator[](int const idx)
{
	assert(idx < m_size);
	return m_data[idx];
}

template<typename T>
T const& static_vector<T>::operator[](int const idx) const
{
	assert(idx < m_size);
	return m_data[idx];
}

template<typename T>
T* static_vector<T>::data()
{
	return m_data;
}

template<typename T>
T const* static_vector<T>::data() const
{
	return m_data;
}

template<typename T>
int static_vector<T>::size() const
{
	return m_size;
}

template<typename T>
T* static_vector<T>::begin()
{
	return m_data;
}

template<typename T>
T const* static_vector<T>::begin() const
{
	return m_data;
}

template<typename T>
T const* static_vector<T>::cbegin() const
{
	return m_data;
}

template<typename T>
T& static_vector<T>::front()
{
	assert(m_size > 0);
	return *m_data;
}

template<typename T>
T const& static_vector<T>::front() const
{
	assert(m_size > 0);
	return *m_data;
}

template<typename T>
T* static_vector<T>::end()
{
	return m_data + m_size;
}

template<typename T>
T const* static_vector<T>::end() const
{
	return m_data + m_size;
}

template<typename T>
T const* static_vector<T>::cend() const
{
	return m_data + m_size;
}
