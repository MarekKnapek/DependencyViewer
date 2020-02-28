#pragma once


#include "smart_menu.h"
#include "tree_window.h"

#include "../nogui/string_converter.h"

#include "../nogui/windows_my.h"


struct file_info;
struct htreeitem_t;
typedef htreeitem_t* htreeitem;


class tree_window_impl
{
private:
	tree_window_impl() noexcept = delete;
	tree_window_impl(HWND const& self);
	tree_window_impl(tree_window_impl const&) = delete;
	tree_window_impl(tree_window_impl&&) noexcept = delete;
	tree_window_impl& operator=(tree_window_impl const&) = delete;
	tree_window_impl& operator=(tree_window_impl&&) noexcept = delete;
	~tree_window_impl();
public:
	static void init();
	static void deinit();
	static wchar_t const* get_class_atom();
private:
	static void register_class();
	static void unregister_class();
	static void create_accel_table();
	static void destroy_accel_table();
	static LRESULT CALLBACK class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam);
	static LRESULT on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam);
	static LRESULT on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_size(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_notify(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_contextmenu(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_command(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_repaint(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_translateaccelerator(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setfi(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_getselection(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_selectitem(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_iscmdpropertiesavail(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setfullpaths(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setonitemchanged(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setcmdmatching(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setcmdproperties(WPARAM const& wparam, LPARAM const& lparam);
	void on_getdispinfow(NMHDR& nmhdr);
	void on_menu(WPARAM const& wparam);
	void on_accelerator(WPARAM const& wparam);
	void on_menu_matching();
	void on_menu_orig();
	void on_menu_prev();
	void on_menu_next();
	void on_menu_expand();
	void on_menu_collapse();
	void on_accel_matching();
	void on_accel_orig();
	void on_accel_prev();
	void on_accel_next();
	void on_accel_expand();
	void on_accel_collapse();
	void on_selchangedw(NMHDR& nmhdr);
	file_info const* get_selection();
	void select_item(file_info const* const& fi);
	smart_menu create_context_menu();
	bool cmd_matching_avail(file_info const* const& fi, file_info const** const& out_fi);
	void cmd_matching();
	bool cmd_orig_avail(file_info const* const& fi, file_info const** const& out_fi);
	void cmd_orig();
	bool cmd_prev_avail(file_info const* const& fi, file_info const** const& out_fi);
	void cmd_prev();
	bool cmd_next_avail(file_info const* const& fi, file_info const** const& out_fi);
	void cmd_next();
	void cmd_expand();
	void cmd_collapse();
	bool cmd_properties_avail(file_info const* const& tmp_fi, wstring_handle* const& out_file_path);
	void refresh(file_info* const& fi);
	void refresh_r(file_info* const& fi, htreeitem const& parent_ti);
	void refresh_e(file_info* const& fi, htreeitem const& parent_ti);
	void repaint();
	bool get_fi_and_point_for_ctx_menu(LPARAM const& lparam, file_info const** const out_fi, POINT* const out_point);
	file_info const* htreeitem_2_file_info(htreeitem const& hti);
private:
	static ATOM g_class;
	static HACCEL g_accel;
private:
	HWND const m_self;
	HWND m_tree_view;
	file_info const* m_fi;
	bool m_fullpaths;
	string_converter m_string_converter;
	smart_menu m_context_menu;
	tree_window::onitemchanged_fn_t m_onitemchanged_fn;
	tree_window::onitemchanged_ctx_t m_onitemchanged_ctx;
	tree_window::cmd_matching_fn_t m_cmd_matching_fn;
	tree_window::cmd_matching_ctx_t m_cmd_matching_ctx;
	tree_window::cmd_properties_fn_t m_cmd_properties_fn;
	tree_window::cmd_properties_ctx_t m_cmd_properties_ctx;
};
