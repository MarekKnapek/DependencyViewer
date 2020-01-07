#pragma once


#include <cstddef>
#include <string>
#include <vector>


typedef std::vector<std::wstring> files_t;


bool parse_files(std::byte const* const data, int const data_len, files_t& out_files);
