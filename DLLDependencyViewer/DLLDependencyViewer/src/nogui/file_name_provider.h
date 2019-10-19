#pragma once


#include "allocator.h"
#include "unique_strings.h"

#include "my_windows.h"
#include <objbase.h>
#include <ole2.h>


class file_name_provider
{
public:
	static void init();
	static void deinit();
	static wstring const* get_correct_file_name(wchar_t const* const& file_name, int const& file_name_len, wunique_strings& us, allocator& alc);
private:
	file_name_provider();
	~file_name_provider();
	wstring const* get_correct_file_name_(wchar_t const* const& file_name, int const& file_name_len, wunique_strings& us, allocator& alc);
private:
	CLSID m_clsid;
	IDispatch* m_object;
	DISPID m_method;
};
