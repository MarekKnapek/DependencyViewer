#pragma once


#include "../nogui/my_string.h"
#include "../nogui/my_string_handle.h"

#include <cstdint>


class string_converter;
struct file_info;
struct modules_list_t;


string_handle const& get_dll_name_no_path(file_info const* const& fi);
bool compare_fi_by_path_or_name(file_info const* const& a, file_info const* const& b);

wstring get_modules_list_col_name(modules_list_t const& modlist, std::uint16_t const& idx, string_converter& str_cnvrtr);
wstring get_modules_list_col_path(modules_list_t const& modlist, std::uint16_t const& idx);
