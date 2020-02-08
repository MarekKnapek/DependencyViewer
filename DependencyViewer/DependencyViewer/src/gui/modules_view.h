#pragma once


#include "../nogui/my_windows.h"


class main_window;


class modules_view
{
public:
	modules_view() = delete;
	modules_view(HWND const parent, main_window& mw);
	modules_view(modules_view const&) = delete;
	modules_view(modules_view&&) noexcept = delete;
	modules_view& operator=(modules_view const&) = delete;
	modules_view& operator=(modules_view&&) noexcept = delete;
	~modules_view();
public:
	HWND const& get_hwnd() const;
	void on_notify(NMHDR& nmhdr);
	void refresh();
	void repaint();
private:
	void on_getdispinfow(NMHDR& nmhdr);
	wchar_t const* on_get_col_name(int const& row);
	wchar_t const* on_get_col_path(int const& row);
private:
	HWND const m_hwnd;
	main_window& m_main_window;
};
