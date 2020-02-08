#include "pe_getters.h"


static /*constexpr*/ string const* const s_export_name_undecorating_p = static_cast<string const*>(nullptr) + 1;
static /*constexpr*/ string const* const s_export_name_processing_p = static_cast<string const*>(nullptr) + 2;
static /*constexpr*/ string_handle const s_export_name_undecorating_h = string_handle{s_export_name_undecorating_p};
static /*constexpr*/ string_handle const s_export_name_processing_h = string_handle{s_export_name_processing_p};


string_handle const& get_name_undecorating()
{
	return s_export_name_undecorating_h;
}

string_handle const& get_export_name_processing()
{
	return s_export_name_processing_h;
}
