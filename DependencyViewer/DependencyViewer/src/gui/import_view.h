#pragma once


#include "smart_menu.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../nogui/string_converter.h"

#include "../nogui/my_windows.h"


class main_window;
struct file_info;
struct pe_import_entry;
struct pe_import_table_info;


class import_view
{
public:
	import_view() = delete;
	import_view(HWND const parent, main_window& mw);
	import_view(import_view const&) = delete;
	import_view(import_view&&) noexcept = delete;
	import_view& operator=(import_view const&) = delete;
	import_view& operator=(import_view&&) noexcept = delete;
	~import_view();
public:
	HWND get_hwnd() const;
	void on_notify(NMHDR& nmhdr);
	void on_getdispinfow(NMHDR& nmhdr);
	void on_columnclick(NMHDR& nmhdr);
	void on_context_menu(LPARAM const lparam);
	void on_menu(std::uint16_t const menu_id);
	void on_menu_matching();
	void on_accel_matching();
	void refresh();
	void repaint();
	void refresh_headers();
	void select_item(std::uint16_t const item_idx);
	void sort_view();
private:
	smart_menu create_menu();
	wchar_t const* on_get_col_type(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	wchar_t const* on_get_col_ordinal(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	wchar_t const* on_get_col_hint(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	wchar_t const* on_get_col_name(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx, file_info const& fi);
	void select_matching_instance();
	int get_type_column_max_width();
private:
	HWND const m_hwnd;
	main_window& m_main_window;
	smart_menu const m_menu;
	std::vector<std::uint16_t> m_sort;
	string_converter m_string_converter;
};
