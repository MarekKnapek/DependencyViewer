#pragma once


#include <cstdio>
#include <string>

#include "cassert_my.h"


template<typename... T>
std::string format_string(char const* const& format, T const&... t)
{
	int const len_1 = std::snprintf(nullptr, 0, format, t...);
	assert(len_1 >= 0);
	std::string ret;
	ret.resize(len_1);
	int const len_2 = std::snprintf(ret.data(), len_1 + 1, format, t...);
	assert(len_2 == len_1);
	return ret;
}

template<typename... T>
std::wstring format_string(wchar_t const* const& format, T const&... t)
{
	std::wstring ret;
	int len = 8;
	for(;;)
	{
		ret.resize(len);
		int const formatted = std::swprintf(ret.data(), len, format, t...);
		if(formatted < 0)
		{
			len = len * 2;
		}
		else
		{
			ret.resize(formatted);
			return ret;
		}
	}
}
