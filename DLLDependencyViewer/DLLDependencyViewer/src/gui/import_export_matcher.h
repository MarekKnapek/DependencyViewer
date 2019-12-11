#pragma once


#include "processor_2.h"
#include "processor_impl_2.h"


void pair_root(file_info_2& fi, tmp_type& to);
void pair_all(file_info_2& fi, tmp_type& to);
void pair_imports_with_exports(file_info_2& fi, file_info_2& sub_fi, tmp_type& to);
void pair_exports_with_imports(file_info_2& fi, file_info_2& sub_fi, tmp_type& to);
