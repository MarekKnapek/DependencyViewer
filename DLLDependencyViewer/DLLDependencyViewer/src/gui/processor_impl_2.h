#pragma once


#include "processor_2.h"

#include "../nogui/memory_manager.h"
#include "../nogui/my_string_handle.h"

#include <string>
#include <vector>


bool process_impl_2(std::vector<std::wstring> const& file_paths, file_info_2& fi, memory_manager& mm);

void step_1(wstring_handle const& origin, file_info_2& fi, memory_manager& mm);
