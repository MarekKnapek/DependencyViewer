#include "unique_strings.h"

#include "allocator.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>


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


void fnv1a_hash_init(std::size_t& state)
{
	state = FNV1_OFFSET;
}

void fnv1a_hash_process(std::size_t& state, void const* const& ptr, int const& len)
{
	for(int i = 0; i != len; ++i)
	{
		state ^= static_cast<std::uint8_t const*>(ptr)[i];
		state *= static_cast<std::size_t>(FNV1_PRIME);
	}
}

std::size_t fnv1a_hash_finish(std::size_t& state)
{
	return state;
}


template<typename char_t>
std::size_t basic_string_hash<char_t>::operator()(basic_string<char_t> const* const& obj) const
{
	std::size_t hash;
	fnv1a_hash_init(hash);
	fnv1a_hash_process(hash, obj->m_str, obj->m_len * sizeof(char_t));
	return fnv1a_hash_finish(hash);
}

template struct basic_string_hash<char>;
template struct basic_string_hash<wchar_t>;


template<typename char_t>
std::size_t basic_string_case_insensitive_hash<char_t>::operator()(basic_string<char_t> const* const& obj) const
{
	std::size_t hash;
	fnv1a_hash_init(hash);
	for(int i = 0; i != obj->m_len; ++i)
	{
		char_t const ch = (obj->m_str[i] | 0b0010'0000);
		fnv1a_hash_process(hash, &ch, 1 * sizeof(char_t));
	}
	return fnv1a_hash_finish(hash);
}

template struct basic_string_case_insensitive_hash<char>;
template struct basic_string_case_insensitive_hash<wchar_t>;


template<typename char_t>
bool basic_string_equal<char_t>::operator()(basic_string<char_t> const* const& a, basic_string<char_t> const* const& b) const
{
	if(a == b)
	{
		return true;
	}
	if(a->m_str == b->m_str)
	{
		assert(a->m_len == b->m_len);
		return true;
	}
	if(a->m_len != b->m_len)
	{
		return false;
	}
	return std::memcmp(a->m_str, b->m_str, a->m_len * sizeof(char_t)) == 0;
}

template struct basic_string_equal<char>;
template struct basic_string_equal<wchar_t>;


template<typename char_t>
bool basic_string_case_insensitive_equal<char_t>::operator()(basic_string<char_t> const* const& a, basic_string<char_t> const* const& b) const
{
	if(a == b)
	{
		return true;
	}
	if(a->m_str == b->m_str)
	{
		assert(a->m_len == b->m_len);
		return true;
	}
	if(a->m_len != b->m_len)
	{
		return false;
	}
	for(int i = 0; i != a->m_len; ++i)
	{
		if((a->m_str[i] | 0b0010'0000) != (b->m_str[i] | 0b0010'0000))
		{
			return false;
		}
	}
	return true;
}

template struct basic_string_case_insensitive_equal<char>;
template struct basic_string_case_insensitive_equal<wchar_t>;


template<typename char_t>
basic_unique_strings<char_t>::basic_unique_strings() noexcept :
	m_strings()
{
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
}

template<typename char_t>
basic_string<char_t> const* const& basic_unique_strings<char_t>::add_string(char_t const* const& str, int const& len, allocator& alc)
{
	basic_string<char_t> const tmp_str{str, len};
	auto const it = m_strings.find(&tmp_str);
	if(it == m_strings.end())
	{
		char_t* const new_buff = alc.allocate_objects<char_t>(len + 1);
		std::memcpy(new_buff, str, len * sizeof(char_t));
		basic_string<char_t>* const new_str = alc.allocate_objects<basic_string<char_t>>(1);
		new_str->m_str = new_buff;
		new_str->m_len = len;
		auto itb = m_strings.insert(new_str);
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
