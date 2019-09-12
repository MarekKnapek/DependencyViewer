#pragma once


#include "splitter_window.h"
#include "processor.h"

#include "../nogui/pe.h"

#include <array>
#include <queue>
#include <string>
#include <utility>

#include <windows.h>


struct _TREEITEM;
typedef struct _TREEITEM* HTREEITEM;
class main_window;
typedef void* idle_task_param_t;
typedef void(* idle_task_t)(main_window&, idle_task_param_t const);


#define wm_main_window_add_idle_task (WM_USER + 0)
#define wm_main_window_process_on_idle (WM_USER + 1)


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
	static HWND create_toolbar(HWND const& parent);
	static LRESULT CALLBACK class_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
private:
	LRESULT on_message(UINT msg, WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_destroy(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_size(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_notify(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_command(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_dropfiles(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_main_window_add_idle_task(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_main_window_process_on_idle(WPARAM wparam, LPARAM lparam);
	LRESULT on_menu(WPARAM wparam, LPARAM lparam);
	LRESULT on_toolbar(WPARAM wparam, LPARAM lparam);
	void on_tree_notify(NMHDR& nmhdr);
	void on_tree_getdispinfow(NMHDR& nmhdr);
	void on_tree_selchangedw(NMHDR& nmhdr);
	void on_tree_dblclk(NMHDR& nmhdr);
	void on_import_notify(NMHDR& nmhdr);
	void on_import_getdispinfow(NMHDR& nmhdr);
	wchar_t const* on_import_get_col_type(pe_import_entry const& import_entry);
	void on_export_notify(NMHDR& nmhdr);
	void on_export_getdispinfow(NMHDR& nmhdr);
	void on_menu_open();
	void on_menu_exit();
	void on_toolbar_open();
	void on_toolbar_full_paths();
	void open();
	void open_file(wchar_t const* const file_path);
	void refresh(main_type&& mo);
	void refresh_view_recursive(file_info& parent_fi, HTREEITEM const& parent_ti);
	int get_import_type_column_max_width();
	int get_export_type_column_max_width();
	int get_twobyte_column_max_width();
	void add_idle_task(idle_task_t const task, idle_task_param_t const param);
	void on_idle();
	void process_command_line();
private:
	static ATOM g_class;
private:
	HWND m_hwnd;
	HWND m_toolbar;
	splitter_window_hor m_splitter_hor;
	HWND m_tree;
	splitter_window_ver m_splitter_ver;
	HWND m_import_list;
	HWND m_export_list;
	std::queue<std::pair<idle_task_t, idle_task_param_t>> m_idle_tasks;
private:
	std::array<std::wstring, 16> m_tmp_strings;
	unsigned m_tmp_string_idx;
private:
	main_type m_mo;
	bool m_full_paths;
};
