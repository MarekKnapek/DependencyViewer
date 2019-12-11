#pragma once


#include "processor.h"
#include "processor_impl.h"


void pair_root(file_info& fi, tmp_type& to);
void pair_all(file_info& fi, tmp_type& to);
void pair_imports_with_exports(file_info& fi, file_info& sub_fi, tmp_type& to);
void pair_exports_with_imports(file_info& fi, file_info& sub_fi, tmp_type& to);
