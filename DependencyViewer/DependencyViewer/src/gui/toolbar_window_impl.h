#pragma once


#include "toolbar_window.h"

#include "../nogui/windows_my.h"


class toolbar_window_impl
{
private:
	toolbar_window_impl() noexcept = delete;
	toolbar_window_impl(HWND const& self);
	toolbar_window_impl(toolbar_window_impl const&) = delete;
	toolbar_window_impl(toolbar_window_impl&&) noexcept = delete;
	toolbar_window_impl& operator=(toolbar_window_impl const&) = delete;
	toolbar_window_impl& operator=(toolbar_window_impl&&) noexcept = delete;
	~toolbar_window_impl();
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
	LRESULT on_wm_notify(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_command(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setfullpathspressed(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setundecoratepressed(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setpropertiesavail(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setcmdopen(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setcmdfullpaths(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setcmdundecorate(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setcmdproperties(WPARAM const& wparam, LPARAM const& lparam);
	void on_getinfotipw(NMHDR& nmhdr);
	void on_toolbar_cmd(WPARAM const& wparam);
	void setfullpathspressed(bool const& pressed);
	void setundecoratepressed(bool const& pressed);
	void setpropertiesavail(bool const& available);
	void cmd_open();
	void cmd_full_paths();
	void cmd_undecorate();
	void cmd_properties();
private:
	static ATOM g_class;
private:
	HWND const m_self;
	HWND m_toolbar;
	toolbar_window::cmd_open_fn_t m_cmd_open_fn;
	toolbar_window::cmd_open_ctx_t m_cmd_open_ctx;
	toolbar_window::cmd_fullpaths_fn_t m_cmd_fullpaths_fn;
	toolbar_window::cmd_fullpaths_ctx_t m_cmd_fullpaths_ctx;
	toolbar_window::cmd_undecorate_fn_t m_cmd_undecorate_fn;
	toolbar_window::cmd_undecorate_ctx_t m_cmd_undecorate_ctx;
	toolbar_window::cmd_properties_fn_t m_cmd_properties_fn;
	toolbar_window::cmd_properties_ctx_t m_cmd_properties_ctx;
};
