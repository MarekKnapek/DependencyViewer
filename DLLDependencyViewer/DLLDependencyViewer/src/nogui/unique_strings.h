#pragma once


#include "my_string_handle.h"

#include <unordered_set>


class allocator;


template<typename char_t>
class basic_unique_strings
{
public:
	basic_unique_strings() noexcept;
	basic_unique_strings(basic_unique_strings<char_t> const&) = delete;
	basic_unique_strings(basic_unique_strings<char_t>&& other) noexcept;
	basic_unique_strings<char_t>& operator=(basic_unique_strings<char_t> const&) = delete;
	basic_unique_strings<char_t>& operator=(basic_unique_strings<char_t>&& other) noexcept;
	~basic_unique_strings() noexcept;
	void swap(basic_unique_strings<char_t>& other) noexcept;
public:
	basic_string_handle<char_t> add_string(char_t const* const str, int const len, allocator& alc);
private:
	std::unordered_set<basic_string_handle<char_t>> m_strings;
};

template<typename char_t> inline void swap(basic_unique_strings<char_t>& a, basic_unique_strings<char_t>& b) noexcept { a.swap(b); }

typedef basic_unique_strings<char> unique_strings;
typedef basic_unique_strings<wchar_t> wunique_strings;
