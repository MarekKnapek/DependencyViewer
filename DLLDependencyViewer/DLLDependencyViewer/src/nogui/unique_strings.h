#pragma once


#include <cstddef>
#include <unordered_set>


class allocator;


template<typename char_t>
struct basic_string
{
	char_t const* m_str;
	int m_len;
};

typedef basic_string<char> string;
typedef basic_string<wchar_t> wstring;


template<typename char_t>
struct basic_string_hash
{
	std::size_t operator()(basic_string<char_t> const& obj) const;
};

typedef basic_string_hash<char> string_hash;
typedef basic_string_hash<wchar_t> wstring_hash;


template<typename char_t>
struct basic_string_equal
{
	bool operator()(basic_string<char_t> const& a, basic_string<char_t> const& b) const;
};

typedef basic_string_equal<char> string_equal;
typedef basic_string_equal<wchar_t> wstring_equal;


template<typename char_t>
class basic_unique_strings
{
public:
	basic_unique_strings() noexcept;
	basic_unique_strings(allocator& alc) noexcept;
	basic_unique_strings(allocator* const& alc) noexcept;
	basic_unique_strings(basic_unique_strings<char_t> const&) = delete;
	basic_unique_strings(basic_unique_strings<char_t>&& other) noexcept;
	basic_unique_strings<char_t>& operator=(basic_unique_strings<char_t> const&) = delete;
	basic_unique_strings<char_t>& operator=(basic_unique_strings<char_t>&& other) noexcept;
	~basic_unique_strings() noexcept;
	void swap(basic_unique_strings<char_t>& other) noexcept;
public:
	basic_string<char_t> const& add_string(char_t const* const& str, int const& len);
private:
	std::unordered_set<basic_string<char_t>, basic_string_hash<char_t>, basic_string_equal<char_t>> m_strings;
	allocator* m_alc;
};

template<typename char_t> inline void swap(basic_unique_strings<char_t>& a, basic_unique_strings<char_t>& b) noexcept { a.swap(b); }

typedef basic_unique_strings<char> unique_strings;
typedef basic_unique_strings<wchar_t> wunique_strings;
