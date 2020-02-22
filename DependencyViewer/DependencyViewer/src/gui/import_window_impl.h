#pragma once


#include "import_window.h"

#include "../nogui/string_converter.h"

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
	static LRESULT CALLBACK class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam);
	static LRESULT on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam);
	static LRESULT on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_size(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_notify(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setfi(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setundecorate(WPARAM const& wparam, LPARAM const& lparam);
	void on_getdispinfow(NMHDR& nmhdr);
	wchar_t const* get_col_type(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	wchar_t const* get_col_ordinal(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	wchar_t const* get_col_hint(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	wchar_t const* get_col_name(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx, pe_export_table_info const& eti);
	std::uint8_t get_col_icon(pe_import_table_info const& iti, std::uint16_t const dll_idx, std::uint16_t const imp_idx);
	void refresh();
	void repaint();
	int get_column_type_max_width();
private:
	static ATOM g_class;
	static int g_column_type_max_width;
private:
	HWND const m_self;
	HWND m_list_view;
	file_info const* m_fi;
	bool m_undecorate;
	string_converter m_string_converter;
};
