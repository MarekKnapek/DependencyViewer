#pragma once


#include "splitter_window.h"

#include "../nogui/pe.h"

#include <array>

#include <windows.h>
#include <commctrl.h>


class main_window
{
public:
	static void register_class();
public:
	main_window();
	main_window(main_window const&) = delete;
	main_window(main_window&&) = delete;
	main_window& operator=(main_window const&) = delete;
	main_window& operator=(main_window&&) = delete;
	~main_window();
public:
	HWND get_hwnd() const;
private:
	static HMENU create_menu();
	static LRESULT CALLBACK class_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
private:
	LRESULT on_message(UINT msg, WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_destroy(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_size(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_notify(WPARAM wparam, LPARAM lparam);
	void on_tree_notify(NMTREEVIEWW& nm);
	void on_import_notify(NMLVDISPINFOW& nm);
	LRESULT on_wm_command(WPARAM wparam, LPARAM lparam);
	LRESULT on_menu(WPARAM wparam, LPARAM lparam);
	void on_menu_open();
	void on_menu_exit();
	void open_file(wchar_t const* const file_name);
	void refresh_view();
private:
	static ATOM m_s_class;
private:
	HWND m_hwnd;
	splitter_window_hor m_splitter_hor;
	HWND m_tree;
	splitter_window_ver m_splitter_ver;
	HWND m_import_list;
	HWND m_export_list;
private:
	std::array<std::wstring, 4> m_tmp_strings;
	unsigned m_tmp_string_idx;
private:
	pe_file_info m_file_info;
};
