#pragma once


#include "smart_menu.h"

#include <cstdint>
#include <string>

#include "../nogui/string_converter.h"

#include "../nogui/my_windows.h"


class main_window;
struct file_info_2;


class tree_view
{
public:
	tree_view() = delete;
	tree_view(HWND const parent, main_window& mw);
	tree_view(tree_view const&) = delete;
	tree_view(tree_view&&) noexcept = delete;
	tree_view& operator=(tree_view const&) = delete;
	tree_view& operator=(tree_view&&) noexcept = delete;
	~tree_view();
public:
	HWND get_hwnd() const;
	void on_notify(NMHDR& nmhdr);
	void on_getdispinfow(NMHDR& nmhdr);
	void on_selchangedw(NMHDR& nmhdr);
	void on_context_menu(LPARAM const lparam);
	void on_menu(std::uint16_t const menu_id);
	void on_menu_orig();
	void on_menu_expand();
	void on_menu_collapse();
	void on_menu_properties();
	void on_accel_orig();
	void on_accel_expand();
	void on_accel_collapse();
	void on_accel_properties();
	void on_toolbar_properties();
	void refresh();
	void repaint();
private:
	smart_menu create_menu();
	void refresh_view_recursive(file_info_2& fi, void* const ti);
	void select_original_instance();
	void expand();
	void collapse();
	void properties();
private:
	HWND const m_hwnd;
	main_window& m_main_window;
	smart_menu const m_menu;
	string_converter m_string_converter;
};
