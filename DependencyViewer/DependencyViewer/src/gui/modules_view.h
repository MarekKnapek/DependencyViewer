#pragma once


#include "../nogui/my_windows.h"
#include "../nogui/string_converter.h"


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
	wstring on_get_col_name(int const& row);
	wstring on_get_col_path(int const& row);
private:
	HWND const m_hwnd;
	main_window& m_main_window;
	string_converter m_string_converter;
};
