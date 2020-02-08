#pragma once


#include "../nogui/my_string_handle.h"


struct file_info;


string_handle const& get_dll_name_no_path(file_info const* const& fi);
bool compare_fi_by_path_or_name(file_info const* const& a, file_info const* const& b);
