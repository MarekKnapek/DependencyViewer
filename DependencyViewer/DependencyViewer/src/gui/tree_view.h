#pragma once


#include "smart_menu.h"

#include <cstdint>
#include <string>

#include "../nogui/string_converter.h"

#include "../nogui/windows_my.h"


class main_window;
struct file_info;
struct htreeitem_t;
typedef htreeitem_t* htreeitem;


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
	void on_menu_match();
	void on_menu_orig();
	void on_menu_prev();
	void on_menu_next();
	void on_menu_expand();
	void on_menu_collapse();
	void on_menu_properties();
	void on_accel_match();
	void on_accel_orig();
	void on_accel_prev();
	void on_accel_next();
	void on_accel_expand();
	void on_accel_collapse();
	void on_accel_properties();
	void refresh();
	void repaint();
	file_info const* get_selection();
private:
	smart_menu create_menu();
	file_info& htreeitem_2_file_info(htreeitem const& hti);
	bool get_fi_and_point_for_context_menu(LPARAM const lparam, file_info const*& out_fi, POINT& out_point);
	std::uint8_t get_tree_item_icon(file_info const& tmp_fi, file_info const* const parent_fi);
	void refresh_view_recursive(file_info& fi, void* const ti);
	void select_match(htreeitem const data = nullptr);
	void select_orig_instance(htreeitem const data = nullptr);
	void select_prev_instance(htreeitem const data = nullptr);
	void select_next_instance(htreeitem const data = nullptr);
	htreeitem get_match_data(file_info const* const curr_fi = nullptr);
	htreeitem get_orig_data(file_info const* const curr_fi = nullptr);
	htreeitem get_prev_data(file_info const* const curr_fi = nullptr);
	htreeitem get_next_data(file_info const* const curr_fi = nullptr);
	void expand();
	void collapse();
	void properties();
private:
	HWND const m_hwnd;
	main_window& m_main_window;
	smart_menu const m_menu;
	string_converter m_string_converter;
};
