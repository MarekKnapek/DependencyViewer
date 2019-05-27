#pragma once


#include "unique_strings.h"
#include "allocator.h"

#define CINTERFACE

#include <windows.h>
#include <ole2.h>
#include <objbase.h>


class file_name_raii
{
public:
	file_name_raii();
	~file_name_raii();
public:
	wstring const* get_correct_file_name(wchar_t const* const& file_name, int const& file_name_len, wunique_strings& us, allocator& alc);
public:
	CLSID m_clsid;
	IDispatch* m_object;
	DISPID m_method;
};
