#pragma once


#include "export_view.h"
#include "import_view.h"
#include "modules_view.h"
#include "processor.h"
#include "settings.h"
#include "smart_menu.h"
#include "splitter_window.h"
#include "tree_view.h"

#include "../nogui/pe.h"
#include "../nogui/thread_worker.h"

#include <array>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "../nogui/my_windows.h"


struct _TREEITEM;
typedef struct _TREEITEM* HTREEITEM;
class main_window;
struct symbols_from_addresses_param_t;
struct undecorated_from_decorated_e_param_t;
struct undecorated_from_decorated_i_param_t;
class dbg_provider;


typedef void* idle_task_param_t;
typedef void(* idle_task_t)(main_window&, idle_task_param_t const);


#define wm_main_window_add_idle_task (WM_USER + 0)
#define wm_main_window_process_on_idle (WM_USER + 1)


class main_window
{
public:
	static void register_class();
	static void create_accel_table();
	static HACCEL get_accell_table();
	static void destroy_accel_table();
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
	LRESULT on_wm_close(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_drawitem(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_notify(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_contextmenu(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_command(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_dropfiles(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_main_window_add_idle_task(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_main_window_process_on_idle(WPARAM wparam, LPARAM lparam);
	void on_menu(WPARAM const wparam);
	void on_menu(std::uint16_t const menu_id);
	void on_accelerator(WPARAM const wparam);
	void on_toolbar(WPARAM const wparam);
	void on_tree_selchangedw();
	void on_modules_itemchanged();
	void on_toolbar_notify(NMHDR& nmhdr);
	void on_menu_open();
	void on_menu_exit();
	void on_menu_paths();
	void on_menu_undecorate();
	void on_menu_refresh();
	void on_menu_properties();
	void on_accel_open();
	void on_accel_paths();
	void on_accel_undecorate();
	void on_accel_properties();
	void on_accel_refresh();
	void on_accel_matching();
	void on_toolbar_open();
	void on_toolbar_full_paths();
	void on_toolbar_undecorate();
	void on_toolbar_properties();
	void commands_availability_refresh();
	void open();
	void open_files(std::vector<std::wstring> const& file_paths);
	void exit();
	void refresh(main_type&& mo);
	void full_paths();
	void properties(wstring_handle data = wstring_handle{});
	wstring_handle get_properties_data(file_info const* const curr_fi = nullptr);
	void undecorate();
	void refresh();
	int get_ordinal_column_max_width();
	std::pair<file_info const*, POINT> get_file_info_2_under_cursor();
	void add_idle_task(idle_task_t const task, idle_task_param_t const param);
	void on_idle();
	void process_command_line();
	void register_dbg_task(thread_worker_param_t const param);
	void unregister_dbg_task(thread_worker_param_t const param);
	void update_staus_bar();
	void draw_status_bar(DRAWITEMSTRUCT& ds);
	void cancel_all_dbg_tasks();
	void request_mo_deletion(std::unique_ptr<main_type>&& mo);
	void request_close();
	void request_symbols_from_addresses(file_info& fi);
	void finish_symbols_from_addresses(symbols_from_addresses_param_t const& param);
	void request_symbol_undecoration(file_info& fi);
	void request_symbol_undecoration_e(file_info& fi, std::vector<std::uint16_t> const& input_indexes);
	void finish_symbol_undecoration_e(undecorated_from_decorated_e_param_t const& param);
	void request_symbol_undecoration_i(file_info& fi, std::uint16_t const dll_idx);
	void finish_symbol_undecoration_i(undecorated_from_decorated_i_param_t const& param);
private:
	static ATOM g_class;
	static HACCEL g_accel;
private:
	HWND m_hwnd;
	HWND m_toolbar;
	splitter_window_hor m_main_panel;
	splitter_window_ver m_upper_panel;
	modules_view m_modules_view;
	tree_view m_tree_view;
	splitter_window_hor m_right_panel;
	import_view m_import_view;
	export_view m_export_view;
	HWND m_status_bar;
	std::queue<std::pair<idle_task_t, idle_task_param_t>> m_idle_tasks;
	std::deque<thread_worker_param_t> m_dbg_tasks;
	main_type m_mo;
	settings m_settings;
private:
	friend class tree_view;
	friend class import_view;
	friend class export_view;
	friend class modules_view;
	template<typename marshaller_t, typename fn_worker_t, typename fn_main_t>
	friend void request_helper(main_window* const self, dbg_provider* const dbg, marshaller_t&& mrshllr, fn_worker_t const fn_worker_, fn_main_t const fn_main_);
};
