#pragma once


namespace list_view_base
{
	int on_columnclick(void const* const param, int const n_headers, int const curr_sort);
	void refresh_headers(void const* const hwnd_ptr, int const n_headers, int const curr_sort);
	int get_selection(void const* const hwnd_ptr);
	void select_item(void const* const hwnd_ptr, void const* const sort_ptr, int const item_idx);
	bool get_context_menu(void const* const hwnd_ptr, void const* const lparam_ptr, void const* const sort_ptr, int* const out_item_idx, void* const out_screen_pos_ptr);
	int get_column_max_width(void const* const hwnd_ptr, void const* const strings_ptr, int const count);
	int get_column_uint16_max_width(void const* const hwnd_ptr);
};
