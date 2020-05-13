#pragma once


#include <cstddef>


template<typename char_t>
struct basic_string
{
public:
	char_t const* begin() const { return m_str; }
	char_t const* cbegin() const { return begin(); }
	char_t const* end() const { return m_str + m_len; }
	char_t const* cend() const { return end(); }
	int size() const { return m_len; }
	explicit operator bool() const { return m_str != nullptr; }
	bool operator!() const { return !this->operator bool(); }
public:
	char_t const* m_str;
	int m_len;
};

template<typename char_t> inline char_t const* begin (basic_string<char_t> const& obj) { return obj.begin (); }
template<typename char_t> inline char_t const* cbegin(basic_string<char_t> const& obj) { return obj.cbegin(); }
template<typename char_t> inline char_t const* end   (basic_string<char_t> const& obj) { return obj.end   (); }
template<typename char_t> inline char_t const* cend  (basic_string<char_t> const& obj) { return obj.cend  (); }

template<typename char_t> inline int size(basic_string<char_t> const& obj) { return obj.size(); }

template<typename char_t> bool operator==(basic_string<char_t> const& a, basic_string<char_t> const& b);
template<typename char_t> bool operator!=(basic_string<char_t> const& a, basic_string<char_t> const& b);
template<typename char_t> bool operator<(basic_string<char_t> const& a, basic_string<char_t> const& b);

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
struct basic_string_case_insensitive_hash
{
	std::size_t operator()(basic_string<char_t> const& obj) const;
};

typedef basic_string_case_insensitive_hash<char> string_case_insensitive_hash;
typedef basic_string_case_insensitive_hash<wchar_t> wstring_case_insensitive_hash;


template<typename char_t>
struct basic_string_equal
{
	bool operator()(basic_string<char_t> const& a, basic_string<char_t> const& b) const;
};

typedef basic_string_equal<char> string_equal;
typedef basic_string_equal<wchar_t> wstring_equal;


template<typename char_t>
struct basic_string_case_insensitive_equal
{
	bool operator()(basic_string<char_t> const& a, basic_string<char_t> const& b) const;
};

typedef basic_string_case_insensitive_equal<char> string_case_insensitive_equal;
typedef basic_string_case_insensitive_equal<wchar_t> wstring_case_insensitive_equal;


template<typename char_t>
struct basic_string_less
{
	bool operator()(basic_string<char_t> const& a, basic_string<char_t> const& b) const;
};

typedef basic_string_less<char> string_less;
typedef basic_string_less<wchar_t> wstring_less;


template<typename char_t>
struct basic_string_case_insensitive_less
{
	bool operator()(basic_string<char_t> const& a, basic_string<char_t> const& b) const;
};

typedef basic_string_case_insensitive_less<char> string_case_insensitive_less;
typedef basic_string_case_insensitive_less<wchar_t> wstring_case_insensitive_less;
