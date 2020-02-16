#pragma once


#include "smart_menu.h"

#include "../nogui/my_windows.h"
#include "../nogui/string_converter.h"

#include <cstdint>
#include <vector>


class main_window;
struct file_info;


class modules_view
{
public:
	modules_view() = delete;
	modules_view(HWND const parent, main_window& mw);
	modules_view(modules_view const&) = delete;
	modules_view(modules_view&&) noexcept = delete;
	modules_view& operator=(modules_view const&) = delete;
	modules_view& operator=(modules_view&&) noexcept = delete;
	~modules_view();
public:
	HWND const& get_hwnd() const;
	void on_notify(NMHDR& nmhdr);
	void refresh();
	void repaint();
	file_info const* get_selection();
	void select_item(file_info const* const& fi);
	void on_context_menu(LPARAM const lparam);
	void on_menu(std::uint16_t const menu_id);
	void on_accel_matching();
	void on_accel_properties();
private:
	smart_menu create_menu();
	void on_getdispinfow(NMHDR& nmhdr);
	void on_columnclick(NMHDR& nmhdr);
	void on_itemchanged(NMHDR& nmhdr);
	wstring on_get_col_name(std::uint16_t const& row);
	wstring on_get_col_path(std::uint16_t const& row);
	wstring on_get_col_name_unsorted(std::uint16_t const& row);
	wstring on_get_col_path_unsorted(std::uint16_t const& row);
	void on_menu_matching();
	void on_menu_properties();
	void matching();
	void properties();
	void refresh_headers();
	void sort_view();
private:
	HWND const m_hwnd;
	main_window& m_main_window;
	smart_menu m_menu;
	string_converter m_string_converter;
	std::uint8_t m_sort_direction;
	std::vector<std::uint16_t> m_sort;
};
