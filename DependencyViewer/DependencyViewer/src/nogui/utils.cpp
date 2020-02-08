#include "utils.h"

#include <cwchar>


template<typename begin_it, typename end_it, typename val_t>
end_it rfind(begin_it const& begin, end_it const& end, val_t const& val)
{
	if(begin == end)
	{
		return end;
	}
	end_it it = end;
	do
	{
		--it;
		if(*it == val)
		{
			return it;
		}
	}
	while (it != begin);
	return end;
}

wchar_t const* find_file_name(wchar_t const* const& file_path, int const& len)
{
	wchar_t const* const it = rfind(file_path, file_path + len, L'\\');
	if(it == file_path + len)
	{
		return file_path;
	}
	else
	{
		return it + 1;
	}
}

wchar_t const* find_file_name(wchar_t const* const& file_path)
{
	return find_file_name(file_path, static_cast<int>(std::wcslen(file_path)));
}
