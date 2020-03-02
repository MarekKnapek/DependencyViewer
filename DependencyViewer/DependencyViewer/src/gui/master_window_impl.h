#pragma once


#include "export_window.h"
#include "import_window.h"
#include "master_window.h"
#include "modules_window.h"
#include "splitter_window.h"
#include "toolbar_window.h"
#include "tree_window.h"

#include "../nogui/windows_my.h"


class master_window_impl
{
private:
	master_window_impl() noexcept = delete;
	master_window_impl(HWND const& self);
	master_window_impl(master_window_impl const&) = delete;
	master_window_impl(master_window_impl&&) noexcept = delete;
	master_window_impl& operator=(master_window_impl const&) = delete;
	master_window_impl& operator=(master_window_impl&&) noexcept = delete;
	~master_window_impl();
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
	void on_size();
	void on_tb_open();
	void on_tb_fullpaths();
	void on_tb_undecorate();
	void on_tb_properties();
	void on_tree_changed(file_info const* const& fi);
	void on_tree_matching(file_info const* const& fi);
	void on_tree_properties(wstring_handle const& file_path);
	void on_imports_matching(std::uint16_t const& item_idx);
	void on_exports_matching(std::uint16_t const& item_idx);
	void on_modules_changed(file_info const* const& fi);
	void on_modules_matching(file_info const* const& fi);
	void on_modules_properties(wstring_handle const& file_path);
private:
	static ATOM g_class;
	static int g_debug_instances;
private:
	HWND const m_self;
	toolbar_window m_toolbar_window;
	splitter_window_hor m_main_panel;
	splitter_window_ver m_upper_panel;
	tree_window m_tree_window;
	splitter_window_hor m_right_panel;
	import_window m_import_window;
	export_window m_export_window;
	modules_window m_modules_window;
};
