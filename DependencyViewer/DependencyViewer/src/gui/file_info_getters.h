#pragma once


#include "../nogui/my_string.h"
#include "../nogui/my_string_handle.h"

#include <cstdint>


class string_converter;
struct file_info;


string_handle const& get_dll_name_no_path(file_info const* const& fi);
bool compare_fi_by_path_or_name_caseinsensitive(file_info const* const& a, file_info const* const& b);

wstring get_tree_fi_name(file_info const* const& tmp_fi, string_converter& str_cnvrtr);
wstring get_tree_fi_path(file_info const* const& tmp_fi, string_converter& str_cnvrtr);

wstring get_modules_fi_name(file_info const* const& fi, string_converter& str_cnvrtr);
wstring get_modules_fi_path(file_info const* const& fi);
