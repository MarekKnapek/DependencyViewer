#pragma once


#include "smart_menu.h"

#include <array>
#include <string>
#include <cstdint>

#include "../nogui/my_windows.h"


class main_window;
struct pe_import_entry;
struct file_info;


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
	void on_context_menu(LPARAM const lparam);
	void on_menu(std::uint16_t const menu_id);
	void on_menu_matching();
	void on_accel_matching();
	void refresh();
	void repaint();
	void select_item(std::uint16_t const item_idx);
private:
	smart_menu create_menu();
	wchar_t const* on_get_col_type(pe_import_entry const& import_entry);
	wchar_t const* on_get_col_ordinal(pe_import_entry const& import_entry, file_info const& fi);
	wchar_t const* on_get_col_hint(pe_import_entry const& import_entry, file_info const& fi);
	wchar_t const* on_get_col_name(pe_import_entry const& import_entry, file_info const& fi);
	void select_matching_instance();
	int get_type_column_max_width();
private:
	HWND const m_hwnd;
	main_window& m_main_window;
	smart_menu const m_menu;
	std::array<std::wstring, 4> m_tmp_strings;
	unsigned m_tmp_string_idx;
};
