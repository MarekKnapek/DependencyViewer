#pragma once


#include "processor.h"
#include "processor_impl.h"


void pair_all(file_info& fi, tmp_type& to);

void pair_imports_with_exports(pe_import_table_info& parent_iti, std::uint16_t const dll_idx, pe_export_table_info const& child_eti, enptr_type const& enpt);
void pair_exports_with_imports(file_info& fi, file_info& sub_fi, tmp_type& to);
