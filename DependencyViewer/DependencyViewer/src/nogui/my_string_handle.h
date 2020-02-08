#pragma once


#include "my_string.h"

#include <functional>


template<typename char_t>
struct basic_string_handle
{
public:
	basic_string<char_t> const* m_string;
public:
	explicit operator bool() const { return m_string != nullptr && *m_string; }
	bool operator!() const { return !this->operator bool(); }
};

template<typename char_t> inline char_t const* begin (basic_string_handle<char_t> const& obj) { return begin (*obj.m_string); }
template<typename char_t> inline char_t const* cbegin(basic_string_handle<char_t> const& obj) { return cbegin(*obj.m_string); }
template<typename char_t> inline char_t const* end   (basic_string_handle<char_t> const& obj) { return end   (*obj.m_string); }
template<typename char_t> inline char_t const* cend  (basic_string_handle<char_t> const& obj) { return cend  (*obj.m_string); }

template<typename char_t> inline int size(basic_string_handle<char_t> const& obj) { return size(*obj.m_string); }

template<typename char_t> inline bool operator==(basic_string_handle<char_t> const& a, basic_string_handle<char_t> const& b) { return basic_string_equal<char_t>{}(*a.m_string, *b.m_string); }
template<typename char_t> inline bool operator!=(basic_string_handle<char_t> const& a, basic_string_handle<char_t> const& b) { return !(a == b); }

template<typename char_t> inline bool operator<(basic_string_handle<char_t> const& a, basic_string_handle<char_t> const& b) { return basic_string_less<char_t>{}(*a.m_string, *b.m_string); }

template<typename char_t> struct basic_string_handle_case_insensitive_hash{ std::size_t operator()(basic_string_handle<char_t> const& obj) const { return basic_string_case_insensitive_hash<char_t>{}(*obj.m_string); } };
typedef basic_string_handle_case_insensitive_hash<char> string_handle_case_insensitive_hash;
typedef basic_string_handle_case_insensitive_hash<wchar_t> wstring_handle_case_insensitive_hash;

template<typename char_t> struct basic_string_handle_case_insensitive_equal{ bool operator()(basic_string_handle<char_t> const& a, basic_string_handle<char_t> const& b) const { return basic_string_case_insensitive_equal<char_t>{}(*a.m_string, *b.m_string); } };
typedef basic_string_handle_case_insensitive_equal<char> string_handle_case_insensitive_equal;
typedef basic_string_handle_case_insensitive_equal<wchar_t> wstring_handle_case_insensitive_equal;

template<typename char_t> struct basic_string_handle_case_insensitive_less{ bool operator()(basic_string_handle<char_t> const& a, basic_string_handle<char_t> const& b) const { return basic_string_case_insensitive_less<char_t>{}(*a.m_string, *b.m_string); } };
typedef basic_string_handle_case_insensitive_less<char> string_handle_case_insensitive_less;
typedef basic_string_handle_case_insensitive_less<wchar_t> wstring_handle_case_insensitive_less;

namespace std { template<> struct hash<basic_string_handle<char>>{ std::size_t operator()(basic_string_handle<char> const& obj) const { return basic_string_hash<char>{}(*obj.m_string); } }; }
namespace std { template<> struct hash<basic_string_handle<wchar_t>>{ std::size_t operator()(basic_string_handle<wchar_t> const& obj) const { return basic_string_hash<wchar_t>{}(*obj.m_string); } }; }

typedef basic_string_handle<char> string_handle;
typedef basic_string_handle<wchar_t> wstring_handle;
