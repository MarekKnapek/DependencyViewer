#pragma once


#include "my_string_handle.h"


template<typename T>
struct optional
{
	T m_value;
	bool m_is_valid;
};


string_handle const& get_name_undecorating();
string_handle const& get_export_name_processing();
