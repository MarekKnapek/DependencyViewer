#include "utils.h"

#include "cassert_my.h"


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

wstring find_file_name(wstring const& file_path)
{
	assert(file_path);
	wchar_t const* const name = find_file_name(file_path.m_str, file_path.m_len);
	assert(name);
	assert(name != file_path.m_str);
	auto const len_ = (file_path.m_str + file_path.m_len) - name;
	assert(len_ >= 0 && len_ <= 0xFFFF);
	int const len = static_cast<int>(len_);
	wstring const ret{name, len};
	return ret;
}

wstring find_file_name(wstring_handle const& file_path)
{
	assert(file_path);
	return find_file_name(*file_path.m_string);
}
