#pragma once


#include "modules_window.h"

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
	static LRESULT CALLBACK class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam);
	static LRESULT on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam);
	static LRESULT on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_size(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_notify(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_repaint(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setmodlist(WPARAM const& wparam, LPARAM const& lparam);
	void on_getdispinfow(NMHDR& nmhdr);
	void on_columnclick(NMHDR& nmhdr);
	wchar_t const* get_col_name(std::uint16_t const& idx);
	wchar_t const* get_col_path(std::uint16_t const& idx);
	bool command_matching_available(std::uint16_t const& item_idx, file_info const** const out_fi);
	void refresh();
	void repaint();
	void sort_view();
private:
	static ATOM g_class;
private:
	HWND const m_self;
	HWND m_list_view;
	modules_list_t const* m_modlist;
	std::uint8_t m_sort_col;
	std::vector<std::uint16_t> m_sort;
	string_converter m_string_converter;
};
