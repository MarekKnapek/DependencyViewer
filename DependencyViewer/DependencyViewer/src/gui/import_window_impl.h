#pragma once


#include "import_window.h"
#include "smart_menu.h"

#include "../nogui/string_converter.h"

#include <cstdint>
#include <vector>

#include "../nogui/windows_my.h"


struct file_info;
struct pe_export_table_info;
struct pe_import_table_info;


class import_window_impl
{
private:
	import_window_impl() noexcept = delete;
	import_window_impl(HWND const& self);
	import_window_impl(import_window_impl const&) = delete;
	import_window_impl(import_window_impl&&) noexcept = delete;
	import_window_impl& operator=(import_window_impl const&) = delete;
	import_window_impl& operator=(import_window_impl&&) noexcept = delete;
	~import_window_impl();
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
	LRESULT on_wm_setundecorate(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_selectitem(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setcmdmatching(WPARAM const& wparam, LPARAM const& lparam);
	void on_getdispinfow(NMHDR& nmhdr);
	void on_columnclick(NMHDR& nmhdr);
	void on_menu(WPARAM const& wparam);
	void on_accelerator(WPARAM const& wparam);
	void on_menu_matching();
	void on_accel_matching();
	wchar_t const* get_col_type(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	wchar_t const* get_col_ordinal(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	wchar_t const* get_col_hint(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	wchar_t const* get_col_name(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx, pe_export_table_info const& eti);
	std::uint8_t get_col_icon(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	smart_menu create_context_menu();
	void select_item(std::uint16_t const& item_idx);
	bool command_matching_available(std::uint16_t const& item_idx, std::uint16_t* const out_item_idx);
	void command_matching();
	void refresh();
	void repaint();
	int get_column_type_max_width();
	void sort_view();
private:
	static ATOM g_class;
	static HACCEL g_accel;
	static int g_column_type_max_width;
private:
	HWND const m_self;
	HWND m_list_view;
	file_info const* m_fi;
	bool m_undecorate;
	std::uint8_t m_sort_col;
	std::vector<std::uint16_t> m_sort;
	string_converter m_string_converter;
	smart_menu m_context_menu;
	import_window::cmd_matching_fn_t m_cmd_matching_fn;
	import_window::cmd_matching_ctx_t m_cmd_matching_ctx;
};
