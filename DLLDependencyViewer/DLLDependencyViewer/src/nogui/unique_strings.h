#pragma once


#include <cstddef>
#include <unordered_set>


class allocator;


template<typename char_t>
struct basic_string
{
public:
	char_t const* begin() const { return m_str; }
	char_t const* cbegin() const { return begin(); }
	char_t const* end() const { return m_str + m_len; }
	char_t const* cend() const { return end(); }
public:
	char_t const* m_str;
	int m_len;
};

template<typename char_t> inline char_t const* begin (basic_string<char_t> const& obj) { return obj.begin (); }
template<typename char_t> inline char_t const* cbegin(basic_string<char_t> const& obj) { return obj.cbegin(); }
template<typename char_t> inline char_t const* end   (basic_string<char_t> const& obj) { return obj.end   (); }
template<typename char_t> inline char_t const* cend  (basic_string<char_t> const& obj) { return obj.cend  (); }
template<typename char_t> inline char_t const* begin (basic_string<char_t> const* const& obj) { return begin (*obj); }
template<typename char_t> inline char_t const* cbegin(basic_string<char_t> const* const& obj) { return cbegin(*obj); }
template<typename char_t> inline char_t const* end   (basic_string<char_t> const* const& obj) { return end   (*obj); }
template<typename char_t> inline char_t const* cend  (basic_string<char_t> const* const& obj) { return cend  (*obj); }

typedef basic_string<char> string;
typedef basic_string<wchar_t> wstring;


template<typename char_t>
struct basic_string_hash
{
	std::size_t operator()(basic_string<char_t> const* const& obj) const;
};

typedef basic_string_hash<char> string_hash;
typedef basic_string_hash<wchar_t> wstring_hash;


template<typename char_t>
struct basic_string_equal
{
	bool operator()(basic_string<char_t> const* const& a, basic_string<char_t> const* const& b) const;
};

typedef basic_string_equal<char> string_equal;
typedef basic_string_equal<wchar_t> wstring_equal;


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
	basic_string<char_t> const* const& add_string(char_t const* const& str, int const& len, allocator& alc);
private:
	std::unordered_set<basic_string<char_t> const*, basic_string_hash<char_t>, basic_string_equal<char_t>> m_strings;
};

template<typename char_t> inline void swap(basic_unique_strings<char_t>& a, basic_unique_strings<char_t>& b) noexcept { a.swap(b); }

typedef basic_unique_strings<char> unique_strings;
typedef basic_unique_strings<wchar_t> wunique_strings;
