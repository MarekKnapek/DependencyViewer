#pragma once


#include "modules_window.h"
#include "smart_menu.h"

#include "../nogui/my_string.h"
#include "../nogui/string_converter.h"

#include <cstdint>
#include <vector>

#include "../nogui/windows_my.h"


struct file_info;
struct modules_list_t;


class modules_window_impl
{
private:
	modules_window_impl() noexcept = delete;
	modules_window_impl(HWND const& self);
	modules_window_impl(modules_window_impl const&) = delete;
	modules_window_impl(modules_window_impl&&) noexcept = delete;
	modules_window_impl& operator=(modules_window_impl const&) = delete;
	modules_window_impl& operator=(modules_window_impl&&) noexcept = delete;
	~modules_window_impl();
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
	LRESULT on_wm_setmodlist(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_selectitem(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_iscmdpropertiesavail(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setonitemchanged(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setcmdmatching(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setcmdproperties(WPARAM const& wparam, LPARAM const& lparam);
	void on_getdispinfow(NMHDR& nmhdr);
	void on_columnclick(NMHDR& nmhdr);
	void on_itemchanged(NMHDR& nmhdr);
	void on_menu(WPARAM const& wparam);
	void on_accelerator(WPARAM const& wparam);
	void on_menu_matching();
	void on_menu_properties();
	void on_accel_matching();
	void on_accel_properties();
	wstring get_col_name_unsorted(std::uint16_t const& idx);
	wstring get_col_name_sorted(std::uint16_t const& idx);
	wstring get_col_path_unsorted(std::uint16_t const& idx);
	wstring get_col_path_sorted(std::uint16_t const& idx);
	smart_menu create_context_menu();
	void select_item(file_info const* const& fi);
	bool command_matching_available(std::uint16_t const& item_idx, file_info const** const out_fi);
	void command_matching();
	bool command_properties_available(std::uint16_t const& item_idx, wstring_handle* const out_str);
	void command_properties();
	void refresh();
	void repaint();
	void sort_view();
private:
	static ATOM g_class;
	static HACCEL g_accel;
private:
	HWND const m_self;
	HWND m_list_view;
	modules_list_t const* m_modlist;
	std::uint8_t m_sort_col;
	std::vector<std::uint16_t> m_sort;
	string_converter m_string_converter;
	smart_menu m_context_menu;
	modules_window::onitemchanged_fn_t m_onitemchanged_fn;
	modules_window::onitemchanged_ctx_t m_onitemchanged_ctx;
	modules_window::cmd_matching_fn_t m_cmd_matching_fn;
	modules_window::cmd_matching_ctx_t m_cmd_matching_ctx;
	modules_window::cmd_properties_fn_t m_cmd_properties_fn;
	modules_window::cmd_properties_ctx_t m_cmd_properties_ctx;
};
