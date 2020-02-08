#include "my_string.h"

#include "fnv1a.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <cwchar>


template<typename char_t>
std::size_t basic_string_hash<char_t>::operator()(basic_string<char_t> const& obj) const
{
	fnv1a_state hash;
	fnv1a_hash_init(hash);
	fnv1a_hash_process(hash, obj.m_str, obj.m_len * sizeof(char_t));
	return fnv1a_hash_finish(hash);
}

template struct basic_string_hash<char>;
template struct basic_string_hash<wchar_t>;


template<typename char_t>
std::size_t basic_string_case_insensitive_hash<char_t>::operator()(basic_string<char_t> const& obj) const
{
	fnv1a_state hash;
	fnv1a_hash_init(hash);
	for(int i = 0; i != obj.m_len; ++i)
	{
		char_t const ch = (obj.m_str[i] | 0b0010'0000);
		fnv1a_hash_process(hash, &ch, 1 * sizeof(char_t));
	}
	return fnv1a_hash_finish(hash);
}

template struct basic_string_case_insensitive_hash<char>;
template struct basic_string_case_insensitive_hash<wchar_t>;


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
bool basic_string_case_insensitive_equal<char_t>::operator()(basic_string<char_t> const& a, basic_string<char_t> const& b) const
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
	for(int i = 0; i != a.m_len; ++i)
	{
		if((a.m_str[i] | 0b0010'0000) != (b.m_str[i] | 0b0010'0000))
		{
			return false;
		}
	}
	return true;
}

template struct basic_string_case_insensitive_equal<char>;
template struct basic_string_case_insensitive_equal<wchar_t>;


template<>
bool basic_string_less<char>::operator()(basic_string<char> const& a, basic_string<char> const& b) const
{
	if(&a == &b)
	{
		return false;
	}
	if(a.m_str == b.m_str)
	{
		assert(a.m_len == b.m_len);
		return false;
	}
	return std::strcmp(a.m_str, b.m_str) < 0;
}

template<>
bool basic_string_less<wchar_t>::operator()(basic_string<wchar_t> const& a, basic_string<wchar_t> const& b) const
{
	if(&a == &b)
	{
		return false;
	}
	if(a.m_str == b.m_str)
	{
		assert(a.m_len == b.m_len);
		return false;
	}
	return std::wcscmp(a.m_str, b.m_str) < 0;
}

template struct basic_string_less<char>;
template struct basic_string_less<wchar_t>;
