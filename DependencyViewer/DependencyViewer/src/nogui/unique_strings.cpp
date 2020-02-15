#include "unique_strings.h"

#include "allocator.h"
#include "cassert_my.h"

#include <cstring>
#include <utility>


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
basic_string_handle<char_t> basic_unique_strings<char_t>::add_string(char_t const* const str, int const len, allocator& alc)
{
	basic_string<char_t> const tmp_str{str, len};
	basic_string_handle<char_t> const tmp_hndl{&tmp_str};
	auto const it = m_strings.find(tmp_hndl);
	if(it == m_strings.end())
	{
		char_t* const new_buff = alc.allocate_objects<char_t>(len + 1);
		std::memcpy(new_buff, str, len * sizeof(char_t));
		new_buff[len] = char_t{'\0'};
		basic_string<char_t>* const new_str = alc.allocate_objects<basic_string<char_t>>(1);
		*new_str = basic_string<char_t>{new_buff, len};
		basic_string_handle<char_t> const new_hndl{new_str};
		auto const itb = m_strings.insert(new_hndl);
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
