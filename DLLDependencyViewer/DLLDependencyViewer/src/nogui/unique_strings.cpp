#include "unique_strings.h"

#include "allocator.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>


std::size_t fnv1a_hash(void const* const& ptr, int const& len)
{
#ifdef _M_IX86
# define FNV1_OFFSET 2166136261uLL
# define FNV1_PRIME 16777619uLL
#else
# ifdef _M_X64
#  define FNV1_OFFSET 14695981039346656037uLL
#  define FNV1_PRIME 1099511628211uLL
# else
#  error Unknown architecture.
# endif
#endif
	std::size_t hash = static_cast<std::size_t>(FNV1_OFFSET);
	for(int i = 0; i != len; ++i)
	{
		hash ^= static_cast<std::uint8_t const*>(ptr)[i];
		hash *= static_cast<std::size_t>(FNV1_PRIME);
	}
	return hash;
}


template<typename char_t>
std::size_t basic_string_hash<char_t>::operator()(basic_string<char_t> const& obj) const
{
	//return std::hash<std::wstring_view>()(std::wstring_view(obj.m_str, obj.m_len));
	return fnv1a_hash(obj.m_str, obj.m_len * sizeof(char_t));
}

template struct basic_string_hash<char>;
template struct basic_string_hash<wchar_t>;


template<typename char_t>
bool basic_string_equal<char_t>::operator()(basic_string<char_t> const& a, basic_string<char_t> const& b) const
{
	if(&a == &b)
	{
		return true;
	}
	if(a.m_str == b.m_str)
	{
		assert(a.m_len == b.m_len);
		return true;
	}
	if(a.m_len != b.m_len)
	{
		return false;
	}
	return std::memcmp(a.m_str, b.m_str, a.m_len * sizeof(char_t)) == 0;
}

template struct basic_string_equal<char>;
template struct basic_string_equal<wchar_t>;


template<typename char_t>
basic_unique_strings<char_t>::basic_unique_strings() noexcept :
	m_strings(),
	m_alc()
{
}

template<typename char_t>
basic_unique_strings<char_t>::basic_unique_strings(allocator& alc) noexcept :
	basic_unique_strings()
{
	m_alc = &alc;
}

template<typename char_t>
basic_unique_strings<char_t>::basic_unique_strings(allocator* const& alc) noexcept :
	basic_unique_strings()
{
	m_alc = alc;
}

template<typename char_t>
basic_unique_strings<char_t>::basic_unique_strings(basic_unique_strings<char_t>&& other) noexcept :
	basic_unique_strings()
{
	swap(other);
}

template<typename char_t>
basic_unique_strings<char_t>& basic_unique_strings<char_t>::operator=(basic_unique_strings<char_t>&& other) noexcept
{
	swap(other);
	return *this;
}

template<typename char_t>
basic_unique_strings<char_t>::~basic_unique_strings() noexcept
{
}

template<typename char_t>
void basic_unique_strings<char_t>::swap(basic_unique_strings& other) noexcept
{
	using std::swap;
	swap(m_strings, other.m_strings);
	swap(m_alc, other.m_alc);
}

template<typename char_t>
basic_string<char_t> const& basic_unique_strings<char_t>::add_string(char_t const* const& str, int const& len)
{
	basic_string<char_t> const tmp_str{str, len};
	auto const it = m_strings.find(tmp_str);
	if(it == m_strings.end())
	{
		char_t* const new_str = m_alc->allocate_objects<char_t>(len + 1);
		std::memcpy(new_str, str, len * sizeof(char_t));
		auto itb = m_strings.insert(basic_string<char_t>{new_str, len});
		assert(itb.first != m_strings.end());
		assert(itb.second);
		return *itb.first;
	}
	else
	{
		return *it;
	}
}

template class basic_unique_strings<char>;
template class basic_unique_strings<wchar_t>;
